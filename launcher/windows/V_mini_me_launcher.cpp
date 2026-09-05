#include "raylib.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
namespace {
struct Config {
    std::string avatar="avatar/default";
    float scale=1.0f;
    int direction=3;
    bool mic=true;
    float threshold=0.035f;
    int releaseMs=250;
    bool bounce=true;
    int bouncePx=4;
    bool bob=false;
    int bobPx=3;
    int bobMs=2400;
    bool sway=false;
    int swayPx=2;
    int swayMs=3000;
    bool setup=true;
    std::array<std::string,8> expressions{};
};
struct Child { PROCESS_INFORMATION pi{}; HANDLE stdinWrite=nullptr; bool active=false; };
struct Picker { bool open=false; bool expression=false; int slot=0; int page=0; std::vector<std::string> folders; };

std::string trim(std::string s){while(!s.empty()&&std::isspace((unsigned char)s.front()))s.erase(s.begin());while(!s.empty()&&std::isspace((unsigned char)s.back()))s.pop_back();return s;}
bool pbool(std::string s,bool f){std::transform(s.begin(),s.end(),s.begin(),[](unsigned char c){return(char)std::tolower(c);});if(s=="1"||s=="true"||s=="yes"||s=="on")return true;if(s=="0"||s=="false"||s=="no"||s=="off")return false;return f;}
const char* dname(int i){static const char*n[]={"horizontal","fourway","quadrants","eightway"};return n[std::clamp(i,0,3)];}
int didx(const std::string&s){if(s=="horizontal")return 0;if(s=="fourway"||s=="4way")return 1;if(s=="quadrants"||s=="quadrant")return 2;return 3;}
std::string exeDir(){char p[MAX_PATH*4]={};DWORD n=GetModuleFileNameA(nullptr,p,(DWORD)sizeof(p));if(!n||n>=sizeof(p))return".";fs::path x(std::string(p,n));return x.parent_path().string();}
std::string join(const std::string&a,const std::string&b){fs::path p(b);return p.is_absolute()?p.string():(fs::path(a)/p).string();}
std::vector<std::string> readLines(const std::string&p){std::ifstream f(p);std::vector<std::string>v;std::string s;while(std::getline(f,s))v.push_back(s);return v;}
void apply(Config&c,const std::string&k,const std::string&v){try{if(k=="avatar_dir")c.avatar=v;else if(k=="scale")c.scale=std::clamp(std::stof(v),.1f,8.f);else if(k=="direction_mode")c.direction=didx(v);else if(k=="mic_enabled")c.mic=pbool(v,c.mic);else if(k=="mic_threshold")c.threshold=std::clamp(std::stof(v),.0001f,1.f);else if(k=="mic_release_ms")c.releaseMs=std::clamp(std::stoi(v),0,5000);else if(k=="talk_bounce")c.bounce=pbool(v,c.bounce);else if(k=="bounce_pixels")c.bouncePx=std::clamp(std::stoi(v),0,200);else if(k=="idle_bob")c.bob=pbool(v,c.bob);else if(k=="idle_bob_pixels")c.bobPx=std::clamp(std::stoi(v),0,200);else if(k=="idle_bob_period_ms")c.bobMs=std::clamp(std::stoi(v),200,30000);else if(k=="idle_sway")c.sway=pbool(v,c.sway);else if(k=="idle_sway_pixels")c.swayPx=std::clamp(std::stoi(v),0,200);else if(k=="idle_sway_period_ms")c.swayMs=std::clamp(std::stoi(v),200,30000);else if(k=="show_setup_on_start")c.setup=pbool(v,c.setup);else if(k.rfind("expression_",0)==0&&k.size()==12&&k[11]>='1'&&k[11]<='8')c.expressions[k[11]-'1']=v;}catch(...){}}
Config load(const std::vector<std::string>&lines){Config c;for(auto&o:lines){auto s=trim(o);if(s.empty()||s[0]=='#'||s[0]==';'||s[0]=='[')continue;auto e=s.find('=');if(e!=std::string::npos)apply(c,trim(s.substr(0,e)),trim(s.substr(e+1)));}return c;}
std::string fstr(float v,int d=3){char b[64];std::snprintf(b,sizeof(b),"%.*f",d,(double)v);return b;}
void setLine(std::vector<std::string>&l,const std::string&k,const std::string&v){for(auto&o:l){auto s=trim(o);if(s.empty()||s[0]=='#'||s[0]==';'||s[0]=='[')continue;auto e=s.find('=');if(e!=std::string::npos&&trim(s.substr(0,e))==k){o=k+"="+v;return;}}l.push_back(k+"="+v);}
void save(const std::string&p,std::vector<std::string>l,const Config&c){setLine(l,"avatar_dir",c.avatar);setLine(l,"scale",fstr(c.scale));setLine(l,"direction_mode",dname(c.direction));setLine(l,"mic_enabled",c.mic?"true":"false");setLine(l,"mic_threshold",fstr(c.threshold,4));setLine(l,"mic_release_ms",std::to_string(c.releaseMs));setLine(l,"talk_bounce",c.bounce?"true":"false");setLine(l,"bounce_pixels",std::to_string(c.bouncePx));setLine(l,"idle_bob",c.bob?"true":"false");setLine(l,"idle_bob_pixels",std::to_string(c.bobPx));setLine(l,"idle_bob_period_ms",std::to_string(c.bobMs));setLine(l,"idle_sway",c.sway?"true":"false");setLine(l,"idle_sway_pixels",std::to_string(c.swayPx));setLine(l,"idle_sway_period_ms",std::to_string(c.swayMs));setLine(l,"show_setup_on_start",c.setup?"true":"false");for(int i=0;i<8;++i)setLine(l,"expression_"+std::to_string(i+1),c.expressions[i]);std::ofstream f(p,std::ios::trunc);for(auto&s:l)f<<s<<'\n';}

bool hit(Rectangle r){return IsMouseButtonPressed(MOUSE_BUTTON_LEFT)&&CheckCollisionPointRec(GetMousePosition(),r);}
bool button(Rectangle r,const std::string&s,bool en=true){bool h=en&&CheckCollisionPointRec(GetMousePosition(),r);DrawRectangleRec(r,en?(h?Color{215,220,225,255}:Color{235,238,240,255}):Color{205,205,205,255});DrawRectangleLinesEx(r,1,DARKGRAY);int w=MeasureText(s.c_str(),15);DrawText(s.c_str(),(int)(r.x+(r.width-w)/2),(int)r.y+7,15,en?BLACK:GRAY);return en&&hit(r);}
bool toggle(Rectangle r,const char*s,bool v){DrawRectangleLinesEx(Rectangle{r.x,r.y+2,20,20},1,DARKGRAY);if(v){DrawLine((int)r.x+4,(int)r.y+12,(int)r.x+9,(int)r.y+18,DARKGREEN);DrawLine((int)r.x+9,(int)r.y+18,(int)r.x+17,(int)r.y+6,DARKGREEN);}DrawText(s,(int)r.x+28,(int)r.y+4,15,BLACK);return hit(r)?!v:v;}
bool field(Rectangle r,std::string&v,bool active){if(hit(r))active=true;if(active&&IsMouseButtonPressed(MOUSE_BUTTON_LEFT)&&!CheckCollisionPointRec(GetMousePosition(),r))active=false;if(active){int ch;while((ch=GetCharPressed())>0)if(ch>=32&&ch<=126&&v.size()<480)v.push_back((char)ch);if(IsKeyPressed(KEY_BACKSPACE)&&!v.empty())v.pop_back();if(IsKeyPressed(KEY_ENTER)||IsKeyPressed(KEY_ESCAPE))active=false;}DrawRectangleRec(r,active?Color{255,255,245,255}:WHITE);DrawRectangleLinesEx(r,active?2:1,active?DARKBLUE:GRAY);std::string s=v;while(MeasureText(s.c_str(),15)>(int)r.width-12&&s.size()>3)s.erase(s.begin());DrawText(s.c_str(),(int)r.x+6,(int)r.y+7,15,BLACK);return active;}
std::vector<std::string> discover(const std::string&dir){std::vector<std::string>o;std::error_code ec;fs::path root=fs::path(dir)/"avatar";if(!fs::is_directory(root,ec))return o;for(const auto&e:fs::directory_iterator(root,ec)){if(ec)break;if(!e.is_directory(ec))continue;if(!fs::is_regular_file(e.path()/"center.png",ec))continue;auto rel=fs::relative(e.path(),fs::path(dir),ec);if(!ec)o.push_back(rel.generic_string());}std::sort(o.begin(),o.end());return o;}
void openPicker(Picker&p,const std::string&dir,bool ex,int slot){p.open=true;p.expression=ex;p.slot=slot;p.page=0;p.folders=discover(dir);}
void drawPicker(Picker&p,Config&c){DrawRectangle(0,0,930,720,Color{0,0,0,110});Rectangle panel{165,105,600,510};DrawRectangleRec(panel,Color{250,250,250,255});DrawRectangleLinesEx(panel,2,DARKGRAY);DrawText(p.expression?"Select expression folder":"Select avatar folder",190,130,22,BLACK);DrawText("Folders under avatar/ that contain center.png",190,162,14,DARKGRAY);const int per=8;int pages=std::max(1,(int)(p.folders.size()+per-1)/per);p.page=std::clamp(p.page,0,pages-1);int a=p.page*per,b=std::min(a+per,(int)p.folders.size());if(p.folders.empty())DrawText("No valid avatar folders found.",190,220,18,MAROON);for(int i=a;i<b;++i){Rectangle r{190.f,200.f+(i-a)*43.f,550.f,34.f};if(button(r,p.folders[i])){if(p.expression)c.expressions[p.slot]=p.folders[i];else c.avatar=p.folders[i];p.open=false;return;}}if(button(Rectangle{190,558,90,34},"Prev",p.page>0))--p.page;DrawText(TextFormat("Page %d / %d",p.page+1,pages),318,567,14,DARKGRAY);if(button(Rectangle{450,558,90,34},"Next",p.page+1<pages))++p.page;if(button(Rectangle{650,558,90,34},"Cancel"))p.open=false;}

void closeChild(Child&c){if(c.stdinWrite){CloseHandle(c.stdinWrite);c.stdinWrite=nullptr;}if(c.pi.hThread){CloseHandle(c.pi.hThread);c.pi.hThread=nullptr;}if(c.pi.hProcess){CloseHandle(c.pi.hProcess);c.pi.hProcess=nullptr;}c.active=false;}
bool running(Child&c){if(!c.active||!c.pi.hProcess)return false;DWORD r=WaitForSingleObject(c.pi.hProcess,0);if(r==WAIT_TIMEOUT)return true;closeChild(c);return false;}
bool sendcmd(Child&c,const std::string&s){if(!c.stdinWrite)return false;DWORD w=0;return WriteFile(c.stdinWrite,s.data(),(DWORD)s.size(),&w,nullptr)&&w==s.size();}
bool spawn(const std::string&dir,Child&c){SECURITY_ATTRIBUTES sa{sizeof(sa),nullptr,TRUE};HANDLE rd=nullptr,wr=nullptr;if(!CreatePipe(&rd,&wr,&sa,0))return false;SetHandleInformation(wr,HANDLE_FLAG_INHERIT,0);std::string logPath=join(dir,"V_mini_me_launcher.log");HANDLE log=CreateFileA(logPath.c_str(),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,&sa,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);if(log!=INVALID_HANDLE_VALUE)SetFilePointer(log,0,nullptr,FILE_END);STARTUPINFOA si{};si.cb=sizeof(si);si.dwFlags=STARTF_USESTDHANDLES;si.hStdInput=rd;si.hStdOutput=log==INVALID_HANDLE_VALUE?GetStdHandle(STD_OUTPUT_HANDLE):log;si.hStdError=si.hStdOutput;std::string exe=join(dir,"V_mini_me.exe");std::string cmd="\""+exe+"\" --no-setup";std::vector<char> buf(cmd.begin(),cmd.end());buf.push_back(0);PROCESS_INFORMATION pi{};BOOL ok=CreateProcessA(exe.c_str(),buf.data(),nullptr,nullptr,TRUE,CREATE_NO_WINDOW,nullptr,dir.c_str(),&si,&pi);CloseHandle(rd);if(log!=INVALID_HANDLE_VALUE)CloseHandle(log);if(!ok){CloseHandle(wr);return false;}c.pi=pi;c.stdinWrite=wr;c.active=true;return true;}
std::string label(const std::string&s){if(s.empty())return"[empty]";auto p=s.find_last_of("/\\");std::string n=p==std::string::npos?s:s.substr(p+1);if(n.size()>12)n.resize(12);return n;}
}

int main(){std::string dir=exeDir(),core=join(dir,"V_mini_me.exe"),cfg=join(dir,"config.ini");auto lines=readLines(cfg);Config c=load(lines);InitWindow(930,720,"V_mini_me optional launcher");SetTargetFPS(30);Child child;Picker picker;bool aedit=false,eedit=false;int slot=0;std::string status=fs::exists(core)?"Ready":"V_mini_me.exe not found beside launcher";
while(!WindowShouldClose()){bool run=running(child);BeginDrawing();ClearBackground(Color{246,246,246,255});DrawText("V_mini_me - optional Windows launcher",22,18,25,BLACK);DrawText(status.c_str(),22,50,15,run?DARKGREEN:DARKGRAY);
DrawText("Avatar folder",22,92,15,BLACK);aedit=field(Rectangle{170,84,610,32},c.avatar,aedit);if(button(Rectangle{790,84,115,32},"Browse avatars"))openPicker(picker,dir,false,0);
DrawText("Scale",22,134,15,BLACK);if(button(Rectangle{170,126,34,28},"-"))c.scale=std::max(.1f,c.scale-.1f);DrawText(fstr(c.scale,2).c_str(),216,134,15,BLACK);if(button(Rectangle{280,126,34,28},"+"))c.scale=std::min(8.f,c.scale+.1f);DrawText("Direction",370,134,15,BLACK);if(button(Rectangle{475,126,180,28},dname(c.direction)))c.direction=(c.direction+1)%4;
c.mic=toggle(Rectangle{22,174,180,26},"Microphone",c.mic);DrawText(("Threshold "+fstr(c.threshold,4)).c_str(),220,180,15,BLACK);if(button(Rectangle{390,172,34,28},"-"))c.threshold=std::max(.0001f,c.threshold-.005f);if(button(Rectangle{430,172,34,28},"+"))c.threshold=std::min(1.f,c.threshold+.005f);DrawText(("Release "+std::to_string(c.releaseMs)+" ms").c_str(),500,180,15,BLACK);if(button(Rectangle{690,172,34,28},"-"))c.releaseMs=std::max(0,c.releaseMs-50);if(button(Rectangle{730,172,34,28},"+"))c.releaseMs=std::min(5000,c.releaseMs+50);
c.bounce=toggle(Rectangle{22,216,190,26},"Talking bounce",c.bounce);DrawText(("Pixels "+std::to_string(c.bouncePx)).c_str(),220,222,15,BLACK);if(button(Rectangle{330,214,34,28},"-"))c.bouncePx=std::max(0,c.bouncePx-1);if(button(Rectangle{370,214,34,28},"+"))c.bouncePx=std::min(200,c.bouncePx+1);
c.bob=toggle(Rectangle{22,258,180,26},"Idle bob",c.bob);DrawText(("Pixels "+std::to_string(c.bobPx)).c_str(),220,264,15,BLACK);if(button(Rectangle{330,256,34,28},"-"))c.bobPx=std::max(0,c.bobPx-1);if(button(Rectangle{370,256,34,28},"+"))c.bobPx=std::min(200,c.bobPx+1);DrawText(("Period "+std::to_string(c.bobMs)+" ms").c_str(),440,264,15,BLACK);if(button(Rectangle{650,256,34,28},"-"))c.bobMs=std::max(200,c.bobMs-100);if(button(Rectangle{690,256,34,28},"+"))c.bobMs=std::min(30000,c.bobMs+100);
c.sway=toggle(Rectangle{22,300,180,26},"Idle sway",c.sway);DrawText(("Pixels "+std::to_string(c.swayPx)).c_str(),220,306,15,BLACK);if(button(Rectangle{330,298,34,28},"-"))c.swayPx=std::max(0,c.swayPx-1);if(button(Rectangle{370,298,34,28},"+"))c.swayPx=std::min(200,c.swayPx+1);DrawText(("Period "+std::to_string(c.swayMs)+" ms").c_str(),440,306,15,BLACK);if(button(Rectangle{650,298,34,28},"-"))c.swayMs=std::max(200,c.swayMs-100);if(button(Rectangle{690,298,34,28},"+"))c.swayMs=std::min(30000,c.swayMs+100);
c.setup=toggle(Rectangle{22,342,300,26},"Terminal setup on direct start",c.setup);DrawLine(22,382,908,382,LIGHTGRAY);DrawText("Quick Expressions",22,398,20,BLACK);if(button(Rectangle{22,432,34,30},"<"))slot=(slot+7)%8;DrawText(("Slot "+std::to_string(slot+1)).c_str(),68,440,15,BLACK);if(button(Rectangle{130,432,34,30},">"))slot=(slot+1)%8;eedit=field(Rectangle{175,430,480,34},c.expressions[slot],eedit);if(button(Rectangle{665,430,110,34},"Browse"))openPicker(picker,dir,true,slot);if(button(Rectangle{785,430,120,34},"Use avatar"))c.expressions[slot]=c.avatar;
DrawText("Live expression buttons",22,485,15,BLACK);for(int i=0;i<8;++i){Rectangle r{22.f+(i%4)*220.f,510.f+(i/4)*38.f,205.f,30.f};bool en=run&&!c.expressions[i].empty();if(button(r,std::to_string(i+1)+": "+label(c.expressions[i]),en)){if(sendcmd(child,"e\n"+std::to_string(i+1)+"\n"))status="Expression sent";}}
DrawLine(22,598,908,598,LIGHTGRAY);if(button(Rectangle{22,620,120,40},"Save")){save(cfg,lines,c);lines=readLines(cfg);status="Saved config.ini";}if(button(Rectangle{155,620,160,40},run?"Restart":"Start + controls",fs::exists(core))){save(cfg,lines,c);lines=readLines(cfg);if(run){sendcmd(child,"q\n");status="Stop requested; press Start again";}else if(spawn(dir,child))status="Avatar started";else status="Could not start";}if(button(Rectangle{328,620,100,40},"Reload",run)){save(cfg,lines,c);lines=readLines(cfg);if(sendcmd(child,"r\n"))status="Reload sent";}if(button(Rectangle{440,620,100,40},"Stop",run)){if(sendcmd(child,"q\n"))status="Stop requested";}DrawText(run?("Runtime PID "+std::to_string((unsigned long)child.pi.dwProcessId)).c_str():"Runtime stopped",22,682,15,DARKGRAY);if(picker.open)drawPicker(picker,c);EndDrawing();}
if(child.active)sendcmd(child,"q\n");closeChild(child);CloseWindow();return 0;}
