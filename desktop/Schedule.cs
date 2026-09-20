using System;
using System.IO;
using System.Linq;
using System.Text;
using System.Collections.Generic;
using System.Threading.Tasks;
using System.Windows.Forms;
using Microsoft.Web.WebView2.Core;
using Microsoft.Web.WebView2.WinForms;

namespace GdipuDesktop
{
    internal sealed partial class MainForm
    {
        const string AcademicMain="https://jw.gdipu.edu.cn/jsxsd/framework/xsMain.jsp";
        readonly Button timetableButton=new Button();
        readonly WebView2 timetableView=new WebView2(),academicView=new WebView2();
        readonly TabPage timetableTab=new TabPage("我的课表"),academicTab=new TabPage("教务系统登录");
        CoreWebView2Environment sharedEnvironment;
        Task timetableInitialization,academicInitialization;
        Dictionary<string,object> schedule;
        bool scheduleBusy;
        string scheduleScript;
        void SetupTimetable(FlowLayoutPanel toolbar)
        {
            SetupButton(timetableButton,"课表",async delegate{try{await EnsureTimetable();tabs.SelectedTab=timetableTab;}catch(Exception e){MessageBox.Show("课表加载失败："+e.Message,Text);}});
            timetableButton.Enabled=false;toolbar.Controls.Add(timetableButton);
            timetableView.Dock=DockStyle.Fill;academicView.Dock=DockStyle.Fill;timetableTab.Controls.Add(timetableView);academicTab.Controls.Add(academicView);
        }
        Task EnsureTimetable(){if(timetableInitialization==null)timetableInitialization=InitializeTimetable();return timetableInitialization;}
        Task EnsureAcademic(){if(academicInitialization==null)academicInitialization=InitializeAcademic();return academicInitialization;}
        async Task InitializeTimetable()
        {
            scheduleScript=ReadResource("timetable.js");
            if(!tabs.TabPages.Contains(timetableTab))tabs.TabPages.Add(timetableTab);tabs.SelectedTab=timetableTab;
            await timetableView.EnsureCoreWebView2Async(sharedEnvironment);
            timetableView.CoreWebView2.WebMessageReceived+=async delegate(object sender,CoreWebView2WebMessageReceivedEventArgs e){
                string errorMessage=null;
                try{string message=e.TryGetWebMessageAsString(),action=message;Dictionary<string,object> payload=null;if(message.StartsWith("{")){payload=Map(json.DeserializeObject(message));action=Str(payload,"action");}if(action=="academic-login"){await EnsureAcademic();tabs.SelectedTab=academicTab;if(academicView.Source==null||academicView.Source.AbsoluteUri=="about:blank")await NavigateAcademic(AcademicMain);}
                else if(action=="import-schedule")await ImportSchedule();else if(action=="save-schedule"&&payload!=null){var saved=Map(payload["data"]);if(saved!=null){schedule=saved;SaveSchedule();}}}catch(Exception ex){errorMessage=ex.Message;}if(errorMessage!=null)await ScheduleProgress(errorMessage);
            };
            string html="<!doctype html><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"><style>body{margin:0}</style><div id=\"app\"></div><script>"+scheduleScript.Replace("</script","<\\/script")+"window.scheduleUI=GdipuTimetable.mount(document.getElementById('app'),null,{login:()=>chrome.webview.postMessage('academic-login'),import:()=>chrome.webview.postMessage('import-schedule'),save:(data)=>chrome.webview.postMessage(JSON.stringify({action:'save-schedule',data}))});</script>";
            var completed=new TaskCompletionSource<bool>();EventHandler<CoreWebView2NavigationCompletedEventArgs> handler=null;handler=delegate{timetableView.CoreWebView2.NavigationCompleted-=handler;completed.TrySetResult(true);};timetableView.CoreWebView2.NavigationCompleted+=handler;timetableView.NavigateToString(html);await completed.Task;
            string cache=Path.Combine(dataDir,"timetable.json");bool cacheFailed=false;if(!testing&&File.Exists(cache)){try{schedule=Map(json.DeserializeObject(File.ReadAllText(cache,Encoding.UTF8)));if(schedule!=null){await timetableView.ExecuteScriptAsync("window.scheduleUI.setData("+json.Serialize(schedule)+")");await ScheduleProgress("已恢复上次导入的课表。需要更新时点击“一键导入课表”。");}}catch{cacheFailed=true;}}if(cacheFailed)await ScheduleProgress("旧课表无法读取，请重新导入。");
            if(!closing)await timetableView.ExecuteScriptAsync("window.scheduleUI.setActivities("+json.Serialize(rows)+")");
        }
        async Task InitializeAcademic()
        {
            if(!tabs.TabPages.Contains(academicTab))tabs.TabPages.Add(academicTab);tabs.SelectedTab=academicTab;
            await academicView.EnsureCoreWebView2Async(sharedEnvironment);academicView.CoreWebView2.Settings.IsPasswordAutosaveEnabled=false;
            academicView.CoreWebView2.NewWindowRequested+=delegate(object s,CoreWebView2NewWindowRequestedEventArgs e){e.Handled=true;academicView.CoreWebView2.Navigate(e.Uri);};
        }
        async Task NavigateAcademic(string url)
        {
            var completed=new TaskCompletionSource<bool>();EventHandler<CoreWebView2NavigationCompletedEventArgs> handler=null;
            handler=delegate(object s,CoreWebView2NavigationCompletedEventArgs e){if(e.IsSuccess)completed.TrySetResult(true);else completed.TrySetException(new Exception("教务页面打开失败："+e.WebErrorStatus));};
            academicView.CoreWebView2.NavigationCompleted+=handler;
            try{academicView.CoreWebView2.Navigate(url);if(await Task.WhenAny(completed.Task,Task.Delay(30000))!=completed.Task)throw new TimeoutException("教务网页加载超时，请在教务页确认网络与登录状态。");await completed.Task;}
            finally{academicView.CoreWebView2.NavigationCompleted-=handler;}
        }
        async Task ScheduleProgress(string text){if(!closing&&timetableView.CoreWebView2!=null)await timetableView.ExecuteScriptAsync("window.scheduleUI.progress("+json.Serialize(text)+")");Log("Timetable: "+text);}
        async Task SyncActivitiesToTimetable(){if(!closing&&timetableInitialization!=null&&timetableInitialization.IsCompleted&&!timetableInitialization.IsFaulted&&timetableView.CoreWebView2!=null)await timetableView.ExecuteScriptAsync("window.scheduleUI.setActivities("+json.Serialize(rows)+")");}
        void SaveSchedule(){if(testing||schedule==null)return;string target=Path.Combine(dataDir,"timetable.json"),temp=target+".tmp";File.WriteAllText(temp,json.Serialize(schedule),new UTF8Encoding(false));if(File.Exists(target))File.Replace(temp,target,null);else File.Move(temp,target);}
        async Task<Dictionary<string,object>> ReadAcademic()
        {
            string script="(()=>{"+scheduleScript+";const data=GdipuTimetable.extract(document);const link=GdipuTimetable.nextLink(document);return {data,link:link?link.label:'',login:!!document.querySelector('input[type=password]'),url:location.href};})()";
            return Map(json.DeserializeObject(await academicView.ExecuteScriptAsync(script)));
        }
        async Task ImportSchedule()
        {
            if(scheduleBusy)return;scheduleBusy=true;Exception failure=null;
            await EnsureTimetable();await timetableView.ExecuteScriptAsync("window.scheduleUI.busy(true)");
            try
            {
                await EnsureAcademic();
                if(academicView.Source==null||academicView.Source.AbsoluteUri=="about:blank")await NavigateAcademic(AcademicMain);
                await ScheduleProgress("正在查找教务系统的个人课表…");
                DateTime deadline=DateTime.UtcNow.AddSeconds(45),stableSince=DateTime.UtcNow;string previous="";var clicked=new HashSet<string>();Dictionary<string,object> imported=null;
                while(DateTime.UtcNow<deadline&&!closing)
                {
                    var state=await ReadAcademic();if(state==null){await Task.Delay(300);continue;}
                    var value=state.ContainsKey("data")?Map(state["data"]):null;
                    if(value!=null){string key=json.Serialize(value);if(key!=previous){previous=key;stableSince=DateTime.UtcNow;}else if((DateTime.UtcNow-stableSince).TotalMilliseconds>1000){imported=value;break;}}
                    else if(object.Equals(state["login"],true)){tabs.SelectedTab=academicTab;throw new Exception("请先在“教务系统登录”页完成登录，再点击课表页的“一键导入课表”。");}
                    else if(!string.IsNullOrEmpty(Str(state,"link"))&&clicked.Add(Str(state,"url")+"|"+Str(state,"link"))){await academicView.ExecuteScriptAsync("(()=>{"+scheduleScript+";const link=GdipuTimetable.nextLink(document);if(link)link.element.click();})()");previous="";}
                    await Task.Delay(300);
                }
                if(closing)return;
                if(imported==null){tabs.SelectedTab=academicTab;throw new Exception("未找到可识别的个人课表。请在教务系统中打开“学生个人课表”，选择学期并查询，再点击“一键导入课表”。原课表未覆盖。");}
                int count=ToRows(imported["courses"]).Count;
                if(count==0)throw new Exception("当前教务课表为空，未覆盖已有课表。请确认学期与查询范围后重试。");
                if(Num(imported,"unknownRows")>0)throw new Exception("部分课程行的节次未能识别，未覆盖已有课表。需要根据实际课表调整导入规则。");
                if(schedule!=null){foreach(string key in new[]{"manualCourses","weekOneMonday","selectedWeek"})if(schedule.ContainsKey(key))imported[key]=schedule[key];}
                imported["importedAt"]=DateTime.UtcNow.ToString("o");Uri source;if(Uri.TryCreate(Str(imported,"source"),UriKind.Absolute,out source))imported["source"]=source.GetLeftPart(UriPartial.Path);
                schedule=imported;
                SaveSchedule();
                await timetableView.ExecuteScriptAsync("window.scheduleUI.setData("+json.Serialize(schedule)+")");tabs.SelectedTab=timetableTab;await ScheduleProgress("已导入 "+count+" 条课程安排，可按周次查看；关闭程序后也会保留。");
            }
            catch(Exception e){failure=e;}
            scheduleBusy=false;if(!closing){await timetableView.ExecuteScriptAsync("window.scheduleUI.busy(false)");if(failure!=null)await ScheduleProgress(failure.Message);}
            if(testing&&failure!=null)throw failure;
        }
        async Task ScheduleSelfTest()
        {
            await EnsureTimetable();await EnsureAcademic();
            if(!string.IsNullOrEmpty(scheduleFixture))
            {
                await NavigateAcademic(scheduleFixture);await ImportSchedule();
                int actualCount=schedule==null?0:ToRows(schedule["courses"]).Count;
                if(actualCount==0)throw new Exception("真实教务课表导入测试未读取到课程");
                Log("PASS actual academic fixture: "+actualCount+" course arrangements");
            }
            await NavigateAcademic("http://127.0.0.1:8765/timetable-main.html");await ImportSchedule();
            if(schedule==null||ToRows(schedule["courses"]).Count!=3)throw new Exception("课表导入测试失败");
            await timetableView.ExecuteScriptAsync("window.scheduleUI.shadow.getElementById('week').value='2';window.scheduleUI.shadow.getElementById('week').dispatchEvent(new Event('change'));");
            string count=json.Deserialize<string>(await timetableView.ExecuteScriptAsync("window.scheduleUI.shadow.getElementById('count').textContent"));if(!count.Contains("2 条"))throw new Exception("单双周筛选不正确："+count);
            await timetableView.ExecuteScriptAsync("(()=>{const s=window.scheduleUI.shadow;s.getElementById('refWeek').value='2';s.getElementById('refDay').value='3';s.getElementById('refDate').value='2026-09-23';s.getElementById('saveDate').click();s.getElementById('courseName').value='临时调课';s.getElementById('teacher').value='测试教师';s.getElementById('place').value='测试教室';s.getElementById('courseDay').value='3';s.getElementById('courseSlot').value='1';s.getElementById('courseWeeks').value='2';s.getElementById('saveCourse').click();})()");
            await Task.Delay(400);string wednesday=json.Deserialize<string>(await timetableView.ExecuteScriptAsync("window.scheduleUI.shadow.getElementById('day3').textContent"));count=json.Deserialize<string>(await timetableView.ExecuteScriptAsync("window.scheduleUI.shadow.getElementById('count').textContent"));int registrationCards=Convert.ToInt32(json.DeserializeObject(await timetableView.ExecuteScriptAsync("window.scheduleUI.shadow.querySelectorAll('.registration-card').length"))),eventCards=Convert.ToInt32(json.DeserializeObject(await timetableView.ExecuteScriptAsync("window.scheduleUI.shadow.querySelectorAll('.event-card').length")));bool hasDeadline=Convert.ToBoolean(json.DeserializeObject(await timetableView.ExecuteScriptAsync("window.scheduleUI.shadow.textContent.includes('报名截止：')")));if(wednesday!="09/23"||!count.Contains("3 条")||!count.Contains("报名提醒")||registrationCards<1||eventCards<1||hasDeadline||schedule==null||!schedule.ContainsKey("manualCourses")||ToRows(schedule["manualCourses"]).Count!=1)throw new Exception("日期、双类型活动提醒或手动课程测试失败");
            tabs.SelectedTab=timetableTab;await Task.Delay(250);using(var reminderOutput=File.Create(Path.Combine(dataDir,"timetable-reminder-preview.png")))await timetableView.CoreWebView2.CapturePreviewAsync(CoreWebView2CapturePreviewImageFormat.Png,reminderOutput);
            await NavigateAcademic("http://127.0.0.1:8765/timetable-main.html");await ImportSchedule();if(!schedule.ContainsKey("manualCourses")||ToRows(schedule["manualCourses"]).Count!=1||Str(schedule,"weekOneMonday")!="2026-09-14")throw new Exception("重新导入后手动课程或日期设置丢失");
            await timetableView.ExecuteScriptAsync("window.scheduleUI.shadow.getElementById('week').value='';window.scheduleUI.shadow.getElementById('week').dispatchEvent(new Event('change'));");
            tabs.SelectedTab=timetableTab;await Task.Delay(400);
            string saved=json.Serialize(schedule);bool emptyRejected=false,loginRejected=false;
            await NavigateAcademic("http://127.0.0.1:8765/timetable-empty.html");try{await ImportSchedule();}catch(Exception e){emptyRejected=e.Message.Contains("为空");}
            await NavigateAcademic("http://127.0.0.1:8765/timetable-login.html");try{await ImportSchedule();}catch(Exception e){loginRejected=e.Message.Contains("登录");}
            if(!emptyRejected||!loginRejected||json.Serialize(schedule)!=saved)throw new Exception("空课表或登录失效保护测试失败");
            Log("PASS timetable: empty table and login expiry preserve previous schedule");
            Log("PASS timetable: separate registration/event reminder cards, dates, manual course persistence, odd/even weeks, 7 days x 6 slots");
        }
    }
}
