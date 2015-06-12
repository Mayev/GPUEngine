#include <limits>
#include"main.h"

void Init();
void Idle();
void Mouse();
void Wheel(int d);

struct SContextParam{
  unsigned    Version;
  std::string Profile;
  std::string Flag;
}ContextParam;

struct SWindowParam{
  unsigned Size[2];
  bool     Fullscreen;
}WindowParam;


bool DisableAnttweakbar=true;
ge::util::ArgumentObject *Args;
ge::util::WindowObject   *Window;
//NDormon::CVertexArray*EmptyVAO;

int main(int Argc,char*Argv[]){
  Args=new ge::util::ArgumentObject(Argc,Argv);

  ShaderDir=Args->getArg("--shader-directory","");

  DisableAnttweakbar=Args->isPresent("--disable-anttweakbar");

  //window args
  WindowParam.Size[0]    = atoi(Args->getArg("-w","800").c_str());
  WindowParam.Size[1]    = atoi(Args->getArg("-h","600").c_str());
  WindowParam.Fullscreen = Args->isPresent("-f");

  //context args
  ContextParam.Version = atoi(Args->getArg("--context-version","430").c_str());
  ContextParam.Profile = Args->getArg("--context-profile","core");
  ContextParam.Flag    = Args->getArg("--context-flag","debug");

  Window=new ge::util::WindowObject(
      WindowParam.Size[0],
      WindowParam.Size[1],
      WindowParam.Fullscreen,
      Idle,
      Mouse,
      !DisableAnttweakbar,
      ContextParam.Version,
      ContextParam.Profile,
      ContextParam.Flag);

  glewExperimental=GL_TRUE;
  glewInit();

  ge::gl::setDefaultDebugMessage();

  //EmptyVAO=new NDormon::CVertexArray();

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glDisable(GL_CULL_FACE);

  Init();
  Window->mainLoop();
  delete Window;
  delete Args;
  return 0;
}

void Mouse(){
}

void Wheel(int /*d*/){
}

void Idle(){
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
  Window->swap();
}

void Init(){

}
