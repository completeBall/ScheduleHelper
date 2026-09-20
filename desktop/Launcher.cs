using System;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Windows.Forms;

internal static class Launcher
{
    [DllImport("kernel32.dll", SetLastError=true, CharSet=CharSet.Unicode)]
    private static extern bool SetDllDirectory(string path);
    [STAThread]
    private static void Main(string[] args)
    {
        Application.EnableVisualStyles();
        Application.SetCompatibleTextRenderingDefault(false);
        try
        {
            string data = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "GdipuActivityHelper");
            for (int i=0;i<args.Length-1;i++) if(args[i]=="--self-test") data=Path.GetFullPath(args[i+1]);
            byte[] application=Resource("App.dll");
            string hash;
            using(var sha=SHA256.Create()) hash=BitConverter.ToString(sha.ComputeHash(application)).Replace("-", "").Substring(0,16);
            string bin=Path.Combine(data,"runtime",hash);Directory.CreateDirectory(bin);
            foreach(string name in new[]{"App.dll","Microsoft.Web.WebView2.Core.dll","Microsoft.Web.WebView2.WinForms.dll","WebView2Loader.dll"})
            {
                byte[] content=Resource(name);string file=Path.Combine(bin,name);
                if(!File.Exists(file) || !Equal(File.ReadAllBytes(file),content)) File.WriteAllBytes(file,content);
            }
            SetDllDirectory(bin);
            AppDomain.CurrentDomain.AssemblyResolve += delegate(object sender, ResolveEventArgs e) {
                string file=Path.Combine(bin,new AssemblyName(e.Name).Name+".dll");
                return File.Exists(file)?Assembly.LoadFrom(file):null;
            };
            Assembly.LoadFrom(Path.Combine(bin,"App.dll")).GetType("GdipuDesktop.Runner").GetMethod("Run").Invoke(null,new object[]{args,data});
        }
        catch(Exception ex)
        {
            Exception error=ex.InnerException??ex;
            if(Array.IndexOf(args,"--self-test")>=0) { Environment.ExitCode=1; return; }
            MessageBox.Show("程序无法启动：\n"+error.Message,"广轻活动汇总",MessageBoxButtons.OK,MessageBoxIcon.Error);
        }
    }
    private static bool Equal(byte[] a,byte[] b){if(a.Length!=b.Length)return false;for(int i=0;i<a.Length;i++)if(a[i]!=b[i])return false;return true;}
    private static byte[] Resource(string name){using(var s=Assembly.GetExecutingAssembly().GetManifestResourceStream(name)){if(s==null)throw new Exception("缺少程序资源："+name);using(var m=new MemoryStream()){s.CopyTo(m);return m.ToArray();}}}
}
