using System;
using System.IO;
using System.Linq;
using System.Text;
using System.Drawing;
using System.Reflection;
using System.Diagnostics;
using System.Collections;
using System.Collections.Generic;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Web.Script.Serialization;
using Microsoft.Web.WebView2.Core;
using Microsoft.Web.WebView2.WinForms;

namespace GdipuDesktop
{
    public static class Runner
    {
        public static void Run(string[] args,string data){Application.Run(new MainForm(args,data));}
    }
    internal sealed partial class MainForm:Form
    {
        const string School="https://study.gdipu.edu.cn";
        readonly string dataDir,baseUrl,core,scheduleFixture;
        readonly bool testing;
        readonly JavaScriptSerializer json=new JavaScriptSerializer{MaxJsonLength=16000000};
        readonly WebView2 website=new WebView2(),dashboard=new WebView2();
        readonly TabControl tabs=new TabControl();
        readonly TabPage tableTab=new TabPage("活动汇总"),schoolTab=new TabPage("学校登录 / 活动页面");
        readonly Button collectButton=new Button(),stopButton=new Button(),loginButton=new Button();
        readonly Label statusLabel=new Label();
        List<Dictionary<string,object>> rows=new List<Dictionary<string,object>>();
        CancellationTokenSource cancellation;
        bool initialized,busy,closing;
        string notice="欢迎使用：首次请点击“学校登录”，登录后再开始采集。",testXlsx;

        public MainForm(string[] args,string data)
        {
            dataDir=data;testing=Array.IndexOf(args,"--self-test")>=0;baseUrl=testing?"http://127.0.0.1:8765/fixture":School;core=ReadResource("core.js");
            for(int i=0;i<args.Length-1;i++)if(args[i]=="--schedule-fixture")scheduleFixture=args[i+1];
            Text="广轻活动汇总";Width=1360;Height=900;MinimumSize=new Size(950,650);StartPosition=FormStartPosition.CenterScreen;Font=new Font("Microsoft YaHei UI",10);
            var top=new FlowLayoutPanel{Dock=DockStyle.Top,Height=64,Padding=new Padding(16,12,12,8),BackColor=Color.FromArgb(244,247,249)};
            SetupButton(loginButton,"学校登录",delegate{if(initialized){tabs.SelectedTab=schoolTab;if(!busy)website.CoreWebView2.Navigate(baseUrl+"/CloudPortal/CloudSquare");}});
            SetupButton(collectButton,"开始采集",async delegate{await CollectClicked();});
            SetupButton(stopButton,"停止采集",delegate{if(cancellation!=null)cancellation.Cancel();});stopButton.Enabled=false;collectButton.Enabled=false;
            var help=new Button();SetupButton(help,"使用说明",delegate{MessageBox.Show("1. 点击“学校登录”，在程序内完成学校统一认证。\n2. 点击“开始采集”，自动读取全部未开始活动。\n3. 在活动汇总页搜索、筛选，点击“导出 Excel”选择保存位置。\n\n团日、班会及班级活动会自动排除。程序会保留自己的登录状态，过期时重新登录即可。\n\n数据保存在：\n"+dataDir+"\n\n只读取活动信息，不自动报名。",Text);});
            top.Controls.AddRange(new Control[]{loginButton,collectButton,stopButton,help});
            SetupTimetable(top);
            statusLabel.Dock=DockStyle.Bottom;statusLabel.Height=34;statusLabel.Padding=new Padding(16,6,8,4);statusLabel.Text="正在启动内置浏览器…";
            website.Dock=DockStyle.Fill;dashboard.Dock=DockStyle.Fill;schoolTab.Controls.Add(website);tableTab.Controls.Add(dashboard);tabs.Dock=DockStyle.Fill;tabs.TabPages.Add(tableTab);tabs.TabPages.Add(schoolTab);
            Controls.Add(tabs);Controls.Add(statusLabel);Controls.Add(top);
            Shown+=async delegate{await Initialize();};
            FormClosing+=delegate{closing=true;if(cancellation!=null)cancellation.Cancel();};
            FormClosed+=delegate{website.Dispose();dashboard.Dispose();timetableView.Dispose();academicView.Dispose();};
            if(testing){ShowInTaskbar=false;WindowState=FormWindowState.Minimized;}
        }
        void SetupButton(Button button,string text,EventHandler handler){button.Text=text;button.AutoSize=true;button.Height=36;button.Padding=new Padding(10,2,10,2);button.Margin=new Padding(0,0,10,0);button.Click+=handler;}
        string ReadResource(string name){using(var stream=Assembly.GetExecutingAssembly().GetManifestResourceStream(name))using(var reader=new StreamReader(stream,Encoding.UTF8))return reader.ReadToEnd();}
        void Log(string text){if(testing)File.AppendAllText(Path.Combine(dataDir,"self-test.log"),DateTime.Now.ToString("HH:mm:ss")+" "+text+Environment.NewLine);}
        async Task Initialize()
        {
            try
            {
                Directory.CreateDirectory(dataDir);
                Log("Creating WebView2 environment");
                var options=new CoreWebView2EnvironmentOptions();options.Language="zh-CN";
                var environment=await CoreWebView2Environment.CreateAsync(null,Path.Combine(dataDir,"BrowserProfile"),options);
                sharedEnvironment=environment;
                Log("Initializing dashboard");
                tabs.SelectedTab=tableTab;await dashboard.EnsureCoreWebView2Async(environment);
                Log("Initializing school browser");
                tabs.SelectedTab=schoolTab;await website.EnsureCoreWebView2Async(environment);tabs.SelectedTab=tableTab;
                Log("Both browser controls initialized");
                website.CoreWebView2.Settings.IsPasswordAutosaveEnabled=false;
                website.CoreWebView2.NewWindowRequested+=delegate(object s,CoreWebView2NewWindowRequestedEventArgs e){e.Handled=true;website.CoreWebView2.Navigate(e.Uri);};
                dashboard.CoreWebView2.NewWindowRequested+=delegate(object s,CoreWebView2NewWindowRequestedEventArgs e){e.Handled=true;if(e.Uri.StartsWith(School+"/CloudPortal/CloudActivityDetail?",StringComparison.Ordinal)){tabs.SelectedTab=schoolTab;website.CoreWebView2.Navigate(e.Uri);}};
                dashboard.CoreWebView2.WebMessageReceived+=async delegate(object s,CoreWebView2WebMessageReceivedEventArgs e){string message=e.TryGetWebMessageAsString(),action=message;Dictionary<string,object> payload=null;if(message.StartsWith("{")){payload=Map(json.DeserializeObject(message));action=Str(payload,"action");}if(action=="collect")await CollectClicked();else if(action=="export-xlsx")ExportExcel(payload!=null&&payload.ContainsKey("data")?ToRows(payload["data"]):rows);};
                dashboard.CoreWebView2.DownloadStarting+=DownloadStarting;
                if(!testing){
                    string cache=Path.Combine(dataDir,"last-results.json");
                    if(File.Exists(cache)){try{rows=ToRows(json.DeserializeObject(File.ReadAllText(cache,Encoding.UTF8))).Where(r=>!IgnoredActivity(r)).ToList();notice="已恢复上次结果。点击“开始采集”获取最新活动。";}catch{notice="上次结果无法读取，请重新采集。";}}
                }
                string html="<!doctype html><html lang=\"zh-CN\"><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"><style>body{margin:0;background:#f4f7f9}</style><div id=\"app\"></div><script>"+core.Replace("</script","<\\/script")+"\nwindow.view=Campus.mount(document.getElementById('app'),[],{refreshLabel:'开始采集',onRefresh:()=>chrome.webview.postMessage('collect'),onExport:(data)=>chrome.webview.postMessage(JSON.stringify({action:'export-xlsx',data}))});</script></html>";
                var loaded=new TaskCompletionSource<bool>();EventHandler<CoreWebView2NavigationCompletedEventArgs> done=null;done=delegate{dashboard.CoreWebView2.NavigationCompleted-=done;loaded.TrySetResult(true);};dashboard.CoreWebView2.NavigationCompleted+=done;dashboard.NavigateToString(html);await loaded.Task;
                initialized=true;collectButton.Enabled=true;await UpdateRows();await Progress(notice);
                timetableButton.Enabled=true;
                Log("WebView2 initialized "+environment.BrowserVersionString);
                if(testing)await SelfTest();
            }
            catch(Exception e)
            {
                Log("FAIL startup: "+e);Environment.ExitCode=1;statusLabel.Text="启动失败："+e.Message;
                if(testing){Close();return;}
                MessageBox.Show("内置浏览器启动失败：\n"+e.Message+"\n\n若电脑缺少 Microsoft Edge WebView2 Runtime，请从微软官网安装后重试：\nhttps://developer.microsoft.com/microsoft-edge/webview2/",Text,MessageBoxButtons.OK,MessageBoxIcon.Error);
            }
        }
        async void DownloadStarting(object sender,CoreWebView2DownloadStartingEventArgs e)
        {
            var deferral=e.GetDeferral();e.Handled=true;
            try
            {
                string suggested=Path.GetFileName(e.ResultFilePath);
                if(string.IsNullOrEmpty(suggested))suggested="活动汇总.html";
                if(testing)e.ResultFilePath=Path.Combine(dataDir,suggested);
                else
                {
                    using(var dialog=new SaveFileDialog{FileName=suggested,Filter="网页文件 (*.html)|*.html",OverwritePrompt=true})
                    {if(dialog.ShowDialog(this)!=DialogResult.OK){e.Cancel=true;return;}e.ResultFilePath=dialog.FileName;}
                }
                var download=e.DownloadOperation;string destination=e.ResultFilePath;
                download.StateChanged+=async delegate{if(download.State==CoreWebView2DownloadState.Completed)await Progress("已导出："+destination);else if(download.State==CoreWebView2DownloadState.Interrupted)await Progress("导出未完成："+download.InterruptReason);};
                await Task.CompletedTask;
            }
            catch(Exception ex){e.Cancel=true;statusLabel.Text="导出失败："+ex.Message;}
            finally{deferral.Complete();}
        }
        async Task Progress(string message){notice=message;if(closing)return;statusLabel.Text=message;Log(message);if(initialized)await dashboard.ExecuteScriptAsync("window.view.progress("+json.Serialize(message)+");");}
        async Task UpdateRows(){if(closing)return;await dashboard.ExecuteScriptAsync("window.view.setRows("+json.Serialize(rows)+");");await SyncActivitiesToTimetable();}
        async Task<object> Eval(string expression){string result=await website.ExecuteScriptAsync("(()=>{"+core+";return "+expression+";})()");return json.DeserializeObject(result);}
        Dictionary<string,object> Map(object value){return value as Dictionary<string,object>;}
        string Str(Dictionary<string,object> obj,string key){return obj!=null&&obj.ContainsKey(key)&&obj[key]!=null?Convert.ToString(obj[key]):"";}
        int Num(Dictionary<string,object> obj,string key){int n;return int.TryParse(Str(obj,key),out n)?n:-1;}
        List<Dictionary<string,object>> ToRows(object value){var array=value as IEnumerable;var result=new List<Dictionary<string,object>>();if(array!=null)foreach(var item in array){var map=Map(item);if(map!=null)result.Add(map);}return result;}
        void CheckLogin(){string url=website.Source==null?"":website.Source.AbsoluteUri;if(url.Contains("/authserver/") || url.Contains("/login")){tabs.SelectedTab=schoolTab;throw new InvalidOperationException("需要登录：请在“学校登录”页完成认证，然后再次点击“开始采集”。");}}
        async Task<Dictionary<string,object>> Until(string expression,Func<Dictionary<string,object>,bool> ready,string description,CancellationToken token,int stable=700,int timeout=25000)
        {
            DateTime start=DateTime.UtcNow,since=start;string last="";
            while((DateTime.UtcNow-start).TotalMilliseconds<timeout)
            {
                token.ThrowIfCancellationRequested();CheckLogin();Dictionary<string,object> result=null;
                try{result=Map(await Eval(expression));}catch{token.ThrowIfCancellationRequested();}
                if(result!=null&&ready(result)){
                    string key=json.Serialize(result);
                    if(key!=last){last=key;since=DateTime.UtcNow;}else if((DateTime.UtcNow-since).TotalMilliseconds>=stable)return result;
                }else{last="";since=DateTime.UtcNow;}
                await Task.Delay(200,token);
            }
            throw new TimeoutException(description+"超时。请确认学校页面能正常打开后重试。");
        }
        const string ListExpression="({url:location.href,ready:document.readyState,cards:Campus.extractCards(document),total:Number(document.querySelector('.el-pagination__total')?.textContent.match(/\\d+/)?.[0]??-1),page:Number(document.querySelector('.el-pager .active')?.textContent||1),pending:[...document.querySelectorAll('.tag-radio-item.active')].some(e=>e.textContent.trim()==='未开始')})";
        async Task Navigate(string url,CancellationToken token)
        {
            var task=new TaskCompletionSource<bool>();
            EventHandler<CoreWebView2NavigationCompletedEventArgs> handler=null;
            handler=delegate(object s,CoreWebView2NavigationCompletedEventArgs e){if(e.IsSuccess)task.TrySetResult(true);else task.TrySetException(new Exception("页面加载失败："+e.WebErrorStatus));};
            website.CoreWebView2.NavigationCompleted+=handler;
            try{website.CoreWebView2.Navigate(url);var winner=await Task.WhenAny(task.Task,Task.Delay(30000,token));token.ThrowIfCancellationRequested();if(winner!=task.Task)throw new TimeoutException("学校网页加载超时");await task.Task;}
            finally{website.CoreWebView2.NavigationCompleted-=handler;}
        }
        async Task<Dictionary<string,object>> OpenList(CancellationToken token)
        {
            // A fresh top-level navigation avoids iframe / CSP restrictions and stale SPA state.
            await Navigate(baseUrl+"/CloudPortal/CloudSquare",token);
            await Until(ListExpression,r=>Num(r,"total")>=0&&(Num(r,"total")==0||ToRows(r["cards"]).Count>0),"活动列表",token,1200);
            object clicked=await Eval("(()=>{const e=[...document.querySelectorAll('.tag-radio-item')].find(e=>e.textContent.trim()==='未开始');if(!e)return false;e.click();return true;})()");
            if(!object.Equals(clicked,true))throw new Exception("未找到“未开始”筛选，请确认当前为学校活动广场。");
            return await Until(ListExpression,r=>object.Equals(r["pending"],true)&&Num(r,"total")>=0&&(Num(r,"total")==0||ToRows(r["cards"]).Count>0),"未开始活动",token,1200);
        }
        async Task<Dictionary<string,object>> Next(Dictionary<string,object> current,CancellationToken token)
        {
            int page=Num(current,"page");string old=json.Serialize(current["cards"]);
            object clicked=await Eval("(()=>{const b=document.querySelector('.el-pagination .btn-next');if(!b||b.disabled)return false;b.click();return true;})()");
            if(!object.Equals(clicked,true))throw new Exception("分页提前结束，请重新采集。");
            return await Until(ListExpression,r=>Num(r,"page")==page+1&&ToRows(r["cards"]).Count>0&&json.Serialize(r["cards"])!=old,"下一页",token);
        }
        string Key(Dictionary<string,object> r){return Str(r,"name")+"|"+Str(r,"teacher")+"|"+Str(r,"activityTime")+"|"+Str(r,"place");}
        bool IgnoredActivity(Dictionary<string,object> r){string name=Str(r,"name");return name.IndexOf("团日",StringComparison.OrdinalIgnoreCase)>=0||name.IndexOf("班会",StringComparison.OrdinalIgnoreCase)>=0||Str(r,"type")=="班级活动";}
        async Task CollectClicked()
        {
            if(!initialized||busy)return;
            busy=true;cancellation=new CancellationTokenSource();var token=cancellation.Token;collectButton.Enabled=false;stopButton.Enabled=true;loginButton.Enabled=false;
            await dashboard.ExecuteScriptAsync("window.view.busy(true)");
            string failureMessage=null;Exception testError=null;
            try
            {
                await Progress("正在读取未开始活动列表…");
                var current=await OpenList(token);int expected=Num(current,"total");if(expected>1000)throw new Exception("活动超过1000项，请确认筛选条件。");
                var queue=new List<Dictionary<string,object>>();
                while(queue.Count<expected){foreach(var r in ToRows(current["cards"])) {r["page"]=Num(current,"page");queue.Add(r);}await Progress("读取列表 "+queue.Count+" / "+expected);if(queue.Count<expected)current=await Next(current,token);}
                if(queue.Count!=expected)throw new Exception("列表数量发生变化，请重新采集。");
                int excluded=queue.Count(r=>IgnoredActivity(r)),excludedClass=0;queue=queue.Where(r=>!IgnoredActivity(r)).ToList();
                rows=new List<Dictionary<string,object>>();await UpdateRows();tabs.SelectedTab=tableTab;var ids=new HashSet<string>();
                for(int i=0;i<queue.Count;i++)
                {
                    var row=queue[i];await Progress("读取详情 "+(i+1)+" / "+queue.Count+"："+Str(row,"name"));current=await OpenList(token);
                    if(Num(current,"total")!=expected)throw new Exception("活动数量改变，请重新采集。");
                    for(int p=1;p<Num(row,"page");p++)current=await Next(current,token);
                    int index=Num(row,"index");var visible=ToRows(current["cards"]);
                    if(index<0||index>=visible.Count||Key(visible[index])!=Key(row))throw new Exception("活动排序改变，请重新采集。");
                    await Eval("(()=>{document.querySelectorAll('.table-container .card-list-item')["+index+"].click();return true;})()");
                    try
                    {
                        string expr="(()=>{if(!location.href.includes('/CloudPortal/CloudActivityDetail?'))return null;try{const r=Campus.extractDetail(document,"+json.Serialize(row)+",location.href);delete r.collectedAt;return r;}catch{return null;}})()";
                        var detail=await Until(expr,r=>!string.IsNullOrEmpty(Str(r,"registration")),"活动详情",token,900);
                        string id=Str(detail,"url");if(!ids.Add(id))throw new InvalidOperationException("详情重复，请重新采集。");
                        detail["collectedAt"]=DateTime.UtcNow.ToString("yyyy-MM-ddTHH:mm:ss.fffZ");if(IgnoredActivity(detail))excludedClass++;else rows.Add(detail);
                    }
                    catch(TimeoutException e){row["error"]=e.Message;row["collectedAt"]=DateTime.UtcNow.ToString("o");rows.Add(row);}
                    await UpdateRows();SaveResults();await Task.Delay(350,token);
                }
                SaveResults();int failed=rows.Count(r=>r.ContainsKey("error")),excludedTotal=excluded+excludedClass;string ignored=excludedTotal>0?"，已排除"+excludedTotal+"项团日/班会/班级活动":"";await Progress(failed==0?"采集完成："+rows.Count+"项"+ignored+"。可搜索、筛选并导出。":"已处理"+rows.Count+"项，其中"+failed+"项读取失败"+ignored+"。");
            }
            catch(OperationCanceledException){website.CoreWebView2.Stop();failureMessage="已停止。已读取的结果可以导出。";}
            catch(Exception e){failureMessage=e.Message;if(testing)testError=e;}
            busy=false;
            if(!closing){collectButton.Enabled=true;stopButton.Enabled=false;loginButton.Enabled=true;await dashboard.ExecuteScriptAsync("window.view.busy(false)");if(failureMessage!=null)await Progress(failureMessage);}
            if(testError!=null)throw testError;
        }
        void SaveResults(){if(testing)return;string target=Path.Combine(dataDir,"last-results.json"),temp=target+".tmp";File.WriteAllText(temp,json.Serialize(rows),new UTF8Encoding(false));if(File.Exists(target))File.Replace(temp,target,null);else File.Move(temp,target);}
        async Task SelfTest()
        {
            try
            {
                await CollectClicked();
                if(rows.Count!=3||rows.Any(r=>r.ContainsKey("error"))||rows.Select(r=>Str(r,"url")).Distinct().Count()!=3||rows.Any(r=>IgnoredActivity(r)))throw new Exception("活动过滤或采集结果不正确");
                if(rows.Count(r=>Str(r,"name")=="同名活动")!=2)throw new Exception("同名活动丢失");
                Log("PASS collection: group-day/class-meeting activities excluded, 3 details retained");
                // Export through the actual desktop UI download handler.
                await dashboard.ExecuteScriptAsync("window.view.shadow.getElementById('export').click()");
                for(int i=0;i<100;i++){if(testXlsx!=null&&File.Exists(testXlsx)&&new FileInfo(testXlsx).Length>0)break;await Task.Delay(100);}
                if(testXlsx==null||!File.Exists(testXlsx)||!VerifyExcel(testXlsx,rows.Count))throw new Exception("Excel 导出验证失败");
                Log("PASS Excel export: styled XLSX, filtered rows sorted by registration start");
                tabs.SelectedTab=tableTab;WindowState=FormWindowState.Normal;await Task.Delay(600);
                using(var file=File.Create(Path.Combine(dataDir,"desktop-preview.png")))await dashboard.CoreWebView2.CapturePreviewAsync(CoreWebView2CapturePreviewImageFormat.Png,file);
                await ScheduleSelfTest();
                File.WriteAllText(Path.Combine(dataDir,"result.json"),json.Serialize(new{pass=true,count=rows.Count,xlsx=testXlsx}),Encoding.UTF8);Log("PASS desktop smoke test");Environment.ExitCode=0;
            }
            catch(Exception e){File.WriteAllText(Path.Combine(dataDir,"result.json"),json.Serialize(new{pass=false,error=e.ToString()}),Encoding.UTF8);Log("FAIL "+e);Environment.ExitCode=1;}
            finally{Close();}
        }
    }
}
