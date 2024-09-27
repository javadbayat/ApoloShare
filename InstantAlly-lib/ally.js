var wshShell = new ActiveXObject("WScript.Shell");
var fso = new ActiveXObject("Scripting.FileSystemObject");
wshShell.CurrentDirectory = fso.GetParentFolderName(location.pathname);

var AllyExec = wshShell.Exec("ally.exe");
AllyExec.StdIn.Write("SYN\n");
var objref = AllyExec.StdOut.ReadLine();
AllyExec.StdIn.Write("SYN/ACK\n");
var AllyService = GetObject(objref);
AllyService.ACK();