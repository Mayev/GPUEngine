#include<geCore/fsa/fsa.h>
#include<sstream>
#include<set>
#include<algorithm>
#include<geCore/disjointSet.h>
#include<geCore/fsa/fusedCallbackData.h>

using namespace ge::core;

void FSA::_initRun(){
  this->_alreadyRead      = "";
  this->_currentChar      = 0 ;
  this->_currentStateName = "";
  this->_currentPosition  = 0 ;
}

FSAState* FSA::_addState(std::string name,bool end){
  if(!this->_name2State.count(name)){
    FSAState*newState=new FSAState(name);
    this->_name2State[name]=newState;
    if(end)this->_endStates.insert(newState);
  }else
    if((this->_endStates.find(this->_name2State[name])!=this->_endStates.end())!=end){
      std::cerr<<"ERROR: FSA::_addFSAState("<<name<<","<<end<<") - state "<<name<<" exists and it is";
      if(end)std::cerr<<" not";
      std::cerr<<" ending state"<<std::endl;
      return nullptr;
    }
  return this->_name2State[name];
}

FSAState*FSA::_getState(std::string name)const{
  return this->_name2State.find(name)->second;
}

std::string FSA::_expandLex(std::string lex)const{
  std::string elex="";
  std::basic_string<char>::size_type len=lex.size();
  char lastChar='\0';
  enum FSAState{
    START         ,
    BACKSLASH     ,
    RANGE         ,
    RANGEBACKSLASH,
  }state=START;
  for(unsigned i=0;i<len;++i){
    char c=lex[i];
    switch(state){
      case START:
        if(c=='\\'){
          if(lastChar!='\0')elex+=lastChar;
          state=BACKSLASH;
          break;
        }else{
          if(lastChar!='\0')elex+=lastChar;
          lastChar=c;
          break;
        }
      case BACKSLASH:
        if(c==FSA::digit[1]){//DIGIT
          for(char i='0';i<='9';++i)elex+=i;
          lastChar='\0';
          state=START;
          break;
        }else if(c==FSA::space[1]){//SPACE
          for(unsigned i=0;i<FSA::space.size();++i)elex+=FSA::space[i];
          lastChar='\0';
          state=START;
          break;
        }else if(c==FSA::range[1]){//DASH
          if(lastChar=='\0'){
            std::cerr<<"left side of range is empty"<<std::endl;
            return"";
          }
          state=RANGE;
          break;
        }else if(c=='\\'){//BACKSLASH
          lastChar=c;
          state=START;
          break;
        }else if(c==FSA::all[1]){//ALL
          for(int i=1;i<=255;++i)elex+=(char)i;
          state=START;
          break;
        }else{
          lastChar=c;
          state=START;
          break;
        }
      case RANGE:
        if(c=='\\'){
          state=RANGEBACKSLASH;
          break;
        }else{
          for(int i=lastChar;i<=c;++i)elex+=(char)i;
          lastChar='\0';
          state=START;
          break;
        }
      case RANGEBACKSLASH:
        if(c==FSA::digit[1]||c==FSA::space[1]||c==FSA::all[1]||c==FSA::range[1]){
          std::cerr<<"range right side cant be \\"<<c<<std::endl;
          return"";
        }else{
          for(int i=lastChar;i<=c;++i)elex+=(char)i;
          lastChar='\0';
          state=START;
          break;
        }
    }

  }
  if(lastChar!='\0')elex+=lastChar;
  std::sort(elex.begin(),elex.end());
  elex.erase(std::unique(elex.begin(),elex.end()),elex.end());
  return elex;
}

void FSA::_computeEndStates(){
  this->_endStates.clear();
  for(auto x:this->_name2State)
    if(x.second->hasEOFTransition())
      this->_endStates.insert(x.second->getEOFTransition().getNextState());
}

void FSA::removeUnreachableStates(){
  std::set<FSAState*>reachable;
  reachable.insert(this->_name2State[this->_start]);
  std::set<ge::core::FSAState*>::size_type oldSize=0;
  std::set<ge::core::FSAState*>::size_type newSize=reachable.size();
  do{
    oldSize=newSize;
    std::set<FSAState*>newFSAStates;
    for(auto x:reachable){
      for(unsigned c=0;c<256;++c)
        if(x->hasTransition((char)c))
          newFSAStates.insert(x->getTransition((char)c).getNextState());
      if(x->getEOFTransition().getNextState())
        newFSAStates.insert(x->getEOFTransition().getNextState());
    }
    for(auto x:newFSAStates)
      reachable.insert(x);
    newSize=reachable.size();
  }while(oldSize!=newSize);
  std::vector<FSAState*>statesToRemove;
  for(auto x:this->_name2State)
    if(reachable.find(x.second)==reachable.end())
      statesToRemove.push_back(x.second);
  this->_endStates.clear();
  this->_name2State.clear();
  for(auto x:statesToRemove)
    delete x;
  for(auto x:reachable){
    this->_name2State[x->getName()]=x;
    if(x->hasEOFTransition())
      this->_endStates.insert(x->getEOFTransition().getNextState());
  }
}

void FSA::removeUndistinguishabeStates(){
  std::vector<ge::core::DisjointSet<FSAState*>>eq;
  eq.push_back(ge::core::DisjointSet<FSAState*>());
  for(auto ip:this->_name2State)
    for(auto iq:this->_name2State){
      auto p=ip.second,q=iq.second;
      if(
          (this->_endStates.find(p)!=this->_endStates.end() &&
           this->_endStates.find(q)!=this->_endStates.end()) ||
          (this->_endStates.find(p)==this->_endStates.end() &&
           this->_endStates.find(q)==this->_endStates.end())
          )
        eq[0].add(p,q);
    }
  do{
    ge::core::DisjointSet<FSAState*>newEq;
    auto lastRel=eq[eq.size()-1];
    for(auto ip:this->_name2State)
      for(auto iq:this->_name2State){
        auto p=ip.second,q=iq.second;
        if(!lastRel.eq(p,q))continue;
        if(p->getNofTransition()!=q->getNofTransition())continue;
        bool transitionsOk=true;
        for(unsigned i=0;i<256;++i){
          char c=(char)i;
          if(p->hasTransition(c)!=q->hasTransition(c)){
            transitionsOk=false;
            break;
          }
          if(!p->hasTransition(c))continue;
          if(p->getTransition(c).getCallback()!=q->getTransition(c).getCallback()){
            transitionsOk=false;
            break;
          }
          if(!lastRel.eq(p->getTransition(c).getNextState(),q->getTransition(c).getNextState())){
            transitionsOk=false;
            break;
          }
        }
        if(!transitionsOk)continue;
        if(q->hasEOFTransition ()!=p->hasEOFTransition ())continue;
        if(p->hasEOFTransition()){
          if(q->getEOFTransition ().getNextState()!=p->getEOFTransition ().getNextState())continue;
          if(q->getEOFTransition ().getCallback ()!=p->getEOFTransition ().getCallback ())continue;
        }
        newEq.add(p,q);
      }
    eq.push_back(newEq);
  }while(!(eq[eq.size()-1]==eq[eq.size()-2]));
  auto lastRel=eq[eq.size()-1];
  /*
  for(auto p:this->_name2State)
    lastRel.add(p.second,p.second);
    */
   
  std::vector<FSAState*>newFSAStates;
  std::map<std::string,FSAState*>newName2State;
  std::map<std::string,std::string>micro2Macro;
  for(auto x:lastRel){
    std::string name="";
    for(auto y:x)
      name+=y->getName();
    //std::cout<<"#"<<name<<"#"<<std::endl;
    if(newName2State.find(name)!=newName2State.end())
      name+="_";
    auto newFSAState=new FSAState(name);
    newFSAStates.push_back(newFSAState);
    newName2State[name]=newFSAState;
    for(auto y:x)
      micro2Macro[y->getName()]=name;
  }

  for(auto ip:this->_name2State)
    for(auto iq:this->_name2State){
      auto p=ip.second,q=iq.second;
      auto pmacro=newName2State[micro2Macro[p->getName()]];
      auto qmacro=newName2State[micro2Macro[q->getName()]];
      for(auto ii:*p)
        if(p->getTransition(ii.first).getNextState()==q)
          pmacro->addTransition(ii.first,qmacro,p->getTransition(ii.first).getCallback());
      if(p->hasEOFTransition()&&p->getEOFTransition().getNextState()==q)
        pmacro->addEOFTransition(qmacro,p->getEOFTransition().getCallback());
    }

  this->_start = newName2State[micro2Macro[this->_start]]->getName();
  for(auto x:this->_name2State)
    delete x.second;
  this->_name2State.clear();
  this->_name2State = newName2State;

  this->_computeEndStates();
}

FSA::FSA(std::string start){
  this->_start = start;
  if(this->_start!="")
    this->_addState(start);
}

FSA::~FSA(){
  for(auto x:this->_name2State)
    delete x.second;
}

void FSA::minimalize(){
  this->removeUnreachableStates();
  this->removeUndistinguishabeStates();
}

bool FSA::_createStates(FSAState**sa,FSAState**sb,std::string nameA,std::string nameB,bool end){
  auto nofFSAStates=this->_name2State.size();
  *sa=this->_addState(nameA);
  if(!*sa)return false;
  if(this->_endStates.find(*sa)!=this->_endStates.end()){
    std::cerr<<"ERROR: can't add transition from EOF state "<<nameA<<" to "<<nameB<<std::endl;
    return false;
  }
  *sb=this->_addState(nameB,end);
  if(!*sb){
    if(this->_name2State.size()!=nofFSAStates){
      this->_name2State.erase(nameA);
      this->_endStates.erase(*sa);
      delete *sa;
    }
    return false;
  }
  return true;
}

void FSA::addTransition(
    std::string      stateA  ,
    char             lex     ,
    std::string      stateB  ,
    FSACallback::Fce callback,
    void*            data    ){
  FSAState*sa=nullptr,*sb=nullptr;
  if(!this->_createStates(&sa,&sb,stateA,stateB))return;
  sa->addTransition(lex,sb,FSACallback(callback,data));
}

void FSA::addTransition(
    std::string      stateA  ,
    std::string      lex     ,
    std::string      stateB  ,
    FSACallback::Fce callback,
    void*            data    ){
  std::string elex=this->_expandLex(lex);
  for(unsigned i=0;i<elex.size();++i){
    this->addTransition(stateA,elex[i],stateB,callback,data);
  }
}

void FSA::addAllTransition(
    std::string      stateA  ,
    std::string      stateB  ,
    FSACallback::Fce callback,
    void*            data    ){
  FSAState*sa=nullptr,*sb=nullptr;
  if(!this->_createStates(&sa,&sb,stateA,stateB))return;
  sa->clearTransitions();
  sa->addElseTransition(sb,FSACallback(callback,data));
}

void FSA::addElseTransition(
    std::string      stateA      ,
    std::string      stateB      ,
    FSACallback::Fce callback,
    void*            data    ){
  FSAState*sa=nullptr,*sb=nullptr;
  if(!this->_createStates(&sa,&sb,stateA,stateB))return;
  sa->addElseTransition(sb,FSACallback(callback,data));
}

void FSA::addEOFTransition(
    std::string      stateA  ,
    std::string      stateB  ,
    FSACallback::Fce callback,
    void*            data    ){
  FSAState*sa=nullptr,*sb=nullptr;
  if(!this->_createStates(&sa,&sb,stateA,stateB,true))return;
  sa->addEOFTransition(sb,FSACallback(callback,data));
}

void FSA::addTransition(
    std::string            stateA  ,
    char                   lex     ,
    std::string            stateB  ,
    FSAFusedCallback const&callback){
  FSAState*sa=nullptr,*sb=nullptr;
  if(!this->_createStates(&sa,&sb,stateA,stateB))return;
  sa->addTransition(lex,sb,callback);
}

void FSA::addTransition(
    std::string            stateA  ,
    std::string            lex     ,
    std::string            stateB  ,
    FSAFusedCallback const&callback){
  std::string elex=this->_expandLex(lex);
  for(unsigned i=0;i<elex.size();++i)
    this->addTransition(stateA,elex[i],stateB,callback);
}

void FSA::addAllTransition(
    std::string            stateA  ,
    std::string            stateB  ,
    FSAFusedCallback const&callback){
  FSAState*sa=nullptr,*sb=nullptr;
  if(!this->_createStates(&sa,&sb,stateA,stateB))return;
  sa->clearTransitions();
  sa->addElseTransition(sb,callback);
}

void FSA::addElseTransition(
    std::string            stateA  ,
    std::string            stateB  ,
    FSAFusedCallback const&callback){
  FSAState*sa=nullptr,*sb=nullptr;
  if(!this->_createStates(&sa,&sb,stateA,stateB))return;
  sa->addElseTransition(sb,callback);
}

void FSA::addEOFTransition(
    std::string            stateA  ,
    std::string            stateB  ,
    FSAFusedCallback const&callback){
  FSAState*sa=nullptr,*sb=nullptr;
  if(!this->_createStates(&sa,&sb,stateA,stateB,true))return;
  sa->addEOFTransition(sb,callback);
}


bool FSA::run(std::string text){
  this->_initRun();
  FSAState*curFSAState=this->_name2State[this->_start];
  this->_currentPosition = 0;
  while(this->_currentPosition<text.size()){
    this->_currentChar      = text[this->_currentPosition];
    this->_currentStateName = curFSAState->getName();
    this->_currentPosition  = this->_currentPosition;
    this->_alreadyRead      = text.substr(0,this->_currentPosition+1);
    FSAState*newFSAState=curFSAState->apply(text[this->_currentPosition],this);
    if(!newFSAState){
      auto ii=this->_state2MessageFce.find(this->_currentStateName);
      if(ii!=this->_state2MessageFce.end())
        ii->second(this,this->_state2MessageData[this->_currentStateName]);
      return false;
    }
    this->_currentPosition++;
    curFSAState=newFSAState;
  }
  if(!curFSAState->getEOFTransition().getNextState()){
    auto ii=this->_state2MessageFce.find(this->_currentStateName);
    if(ii!=this->_state2MessageFce.end())
      ii->second(this,this->_state2MessageData[this->_currentStateName]);
    return false;
  }
  curFSAState->getEOFTransition().callCallback(this);
  return true;
}

char FSA::getCurrentChar()const{
  return this->_currentChar;
}

std::string FSA::getAlreadyReadString()const{
  return this->_alreadyRead;
}

std::string FSA::getCurrentStateName()const{
  return this->_currentStateName;
}

unsigned FSA::getCurrentPosition()const{
  return this->_currentPosition;
}

void FSA::goBack(){
  this->_currentPosition--;
}

std::string FSA::toStr()const{
  std::stringstream ss;
  for(auto x:this->_name2State)
    ss<<x.first<<": "<<x.second->toStr()<<std::endl;
  return ss.str();
}

FSA FSA::operator+(FSA const&other)const{
  FSA result;

  std::map<std::string,FSAState*>newStates;
  std::map<std::pair<std::string,std::string>,std::string>micro2Macro;
  std::map<std::string,std::string>micro2newMicro[2];
  for(auto ip:this->_name2State)
    for(auto iq:other._name2State){
      auto p=ip.second->getName(),q=iq.second->getName();
      std::string name=p+q;
      while(newStates.find(name)!=newStates.end())name+="_";
      if(p==this->_start&&q==other._start)result._start = name;
      newStates[name]=new FSAState(name);
      micro2Macro[std::pair<std::string,std::string>(p,q)]=name;
    }
  FSA const to[2]={*this,other};
  for(unsigned i=0;i<2;++i)
    for(auto ip:to[i]){
      auto p=ip.second->getName();
      std::string name=p;
      while(newStates.find(name)!=newStates.end())name+="_";
      newStates[name]=new FSAState(name);
      micro2newMicro[i][p]=name;
    }

#define DO_TRANSITION(hasFCE,getFCE)\
  if(p->hasFCE){\
    auto pnn=p->getFCE.getNextState()->getName();\
    if(q->hasFCE){\
      auto qnn=q->getFCE.getNextState()->getName();\
      from = newStates[micro2Macro[std::pair<std::string,std::string>(p->getName(),q->getName())]];\
      to   = newStates[micro2Macro[std::pair<std::string,std::string>(pnn,qnn)]];\
      callback = FSAFusedCallback(p->getFCE.getCallback(),q->getFCE.getCallback());\
    }else{\
      from = newStates[micro2newMicro[0][p->getName()]];\
      to   = newStates[micro2newMicro[0][pnn         ]];\
      callback = p->getFCE.getCallback();\
    }\
  }else{\
    if(q->hasFCE){\
      auto qnn=q->getFCE.getNextState()->getName();\
      from = newStates[micro2newMicro[1][q->getName()]];\
      to   = newStates[micro2newMicro[1][qnn         ]];\
      callback = q->getFCE.getCallback();\
    }else continue;\
  }

  for(auto ip:this->_name2State)
    for(auto iq:other._name2State){
      auto p=ip.second,q=iq.second;
      for(unsigned i=0;i<256;++i){
        char c=(char)i;
        FSAState*from=nullptr,*to=nullptr;
        FSAFusedCallback callback;
        DO_TRANSITION(hasTransition(c),getTransition(c));
        from->addTransition(c,to,callback);
      }
      FSAState*from=nullptr,*to=nullptr;
      FSAFusedCallback callback;
      DO_TRANSITION(hasEOFTransition(),getEOFTransition());
      if(from)from->addEOFTransition(to,callback);
    }
  result._name2State = newStates;
  result._computeEndStates();

  result.minimalize();
  return result;
}
/*
   FSA const& FSA::operator*(FSA const&other)const{
   }
   */
FSA::Iterator FSA::begin()const{
  return this->_name2State.begin();
}

FSA::Iterator FSA::end()const{
  return this->_name2State.end();
}

const std::string FSA::els   = ""       ;
const std::string FSA::eof   = "\\e"    ;
const std::string FSA::digit = "\\d"    ;
const std::string FSA::range = "\\-"    ;
const std::string FSA::all   = "\\."    ;
const std::string FSA::space = " \t\r\n";

