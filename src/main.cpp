#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <fstream>

struct Stroke{std::vector<sf::Vector2f>points;sf::Color color{sf::Color::Black};float radius{6.f};bool eraser{false};};
static float dist(sf::Vector2f a,sf::Vector2f b){auto dx=a.x-b.x,dy=a.y-b.y;return std::sqrt(dx*dx+dy*dy);}
static void drawStroke(sf::RenderTarget& t,const Stroke& s,const sf::Color& bg){
  if(s.points.empty())return; sf::Color c=s.eraser?bg:s.color; sf::CircleShape dot(s.radius); dot.setOrigin(s.radius,s.radius); dot.setFillColor(c);
  if(s.points.size()==1){dot.setPosition(s.points[0]); t.draw(dot); return;}
  for(size_t i=1;i<s.points.size();++i){auto p0=s.points[i-1],p1=s.points[i]; float d=dist(p0,p1),step=std::max(1.f,s.radius*0.6f); int steps=int(d/step)+1; auto dir=(p1-p0)/float(steps);
    for(int k=0;k<=steps;++k){auto p=p0+dir*float(k); dot.setPosition(p); t.draw(dot);} }
}
static bool savePNG(const std::vector<Stroke>& S,const sf::Color& bg,const sf::Vector2u& size){
  sf::RenderTexture rt; if(!rt.create(size.x,size.y))return false; rt.clear(bg); for(auto& s:S) drawStroke(rt,s,bg); rt.display();
  auto img=rt.getTexture().copyToImage(); std::time_t t=std::time(nullptr); std::tm tm{};
#if defined(_WIN32)
  localtime_s(&tm,&t);
#else
  localtime_r(&t,&tm);
#endif
  std::ostringstream oss; oss<<"whiteboard_"<<std::put_time(&tm,"%Y-%m-%d_%H-%M-%S")<<".png"; return img.saveToFile(oss.str());
}
static bool saveJSON(const std::vector<Stroke>& S,const std::string& fn){
  std::ofstream f(fn); if(!f) return false; f<<"{\n  \"strokes\": [\n";
  for(size_t i=0;i<S.size();++i){auto&s=S[i]; f<<"    {\n      \"radius\": "<<s.radius<<",\n      \"eraser\": "<<(s.eraser?"true":"false")<<",\n      \"color\": ["<<(int)s.color.r<<","<<(int)s.color.g<<","<<(int)s.color.b<<","<<(int)s.color.a<<"],\n      \"points\": [";
    for(size_t j=0;j<s.points.size();++j){f<<"["<<s.points[j].x<<","<<s.points[j].y<<"]"<<(j+1<s.points.size()?",":"");} f<<"]\n    }"<<(i+1<S.size()?",":"")<<"\n";}
  f<<"  ]\n}\n"; return true;
}
static bool loadJSON(std::vector<Stroke>& out,const std::string& fn){
  std::ifstream f(fn); if(!f) return false; std::string s((std::istreambuf_iterator<char>(f)),{}); out.clear(); size_t p=0;
  auto next=[&](const std::string& tok){size_t q=s.find(tok,p); if(q==std::string::npos) return q; p=q+tok.size(); return q;};
  if(next("\"strokes\"")==std::string::npos||next("[")==std::string::npos) return false;
  while(true){size_t os=s.find("{",p), ae=s.find("]",p); if(ae!=std::string::npos && (os==std::string::npos||ae<os)) break; if(os==std::string::npos) break; p=os+1; Stroke st;
    if(next("\"radius\"")==std::string::npos||next(":")==std::string::npos) return false; st.radius=std::stof(s.substr(p));
    if(next("\"eraser\"")==std::string::npos||next(":")==std::string::npos) return false; st.eraser=(s.compare(p,4,"true")==0);
    if(next("\"color\"")==std::string::npos||next("[")==std::string::npos) return false; int r=std::stoi(s.substr(p)); if(next(",")==std::string::npos) return false;
      int g=std::stoi(s.substr(p)); if(next(",")==std::string::npos) return false; int b=std::stoi(s.substr(p)); if(next(",")==std::string::npos) return false;
      int a=std::stoi(s.substr(p)); if(next("]")==std::string::npos) return false; st.color=sf::Color((sf::Uint8)r,(sf::Uint8)g,(sf::Uint8)b,(sf::Uint8)a);
    if(next("\"points\"")==std::string::npos||next("[")==std::string::npos) return false;
    while(true){size_t mc=s.find("]",p), mo=s.find("[",p); if(mc!=std::string::npos && (mo==std::string::npos||mc<mo)){p=mc+1; break;}
      if(mo==std::string::npos) break; p=mo+1; float x=std::stof(s.substr(p)); if(next(",")==std::string::npos) return false; float y=std::stof(s.substr(p));
      if(next("]")==std::string::npos) return false; st.points.push_back({x,y}); size_t comma=s.find(",",p), close=s.find("]",p);
      if(close!=std::string::npos && (comma==std::string::npos||close<comma)) {p=close;} else if(comma!=std::string::npos) {p=comma+1;} else break;}
    out.push_back(std::move(st)); size_t no=s.find("{",p), arrEnd=s.find("]",p); if(arrEnd!=std::string::npos && (no==std::string::npos||arrEnd<no)){p=arrEnd+1; break;}
  }
  return true;
}
int main(){const unsigned W=1280,H=800; const sf::Color BG(250,250,250); sf::ContextSettings s; s.antialiasingLevel=8;
  sf::RenderWindow win(sf::VideoMode(W,H),"Whiteboard (C++/SFML)",sf::Style::Default,s); win.setVerticalSyncEnabled(true);
  float radius=6.f; std::vector<sf::Color> pal={sf::Color::Black,{230,57,70},{29,53,87},{69,123,157},{43,161,69},{244,162,97},{255,210,63}}; size_t ci=0; sf::Color color=pal[ci]; bool eraser=false;
  std::vector<Stroke> strokes,redo; Stroke cur; bool drawing=false; const float minStep=0.8f; sf::Font font; font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"); sf::Text hud;
  if(font.getInfo().family!=""){hud.setFont(font); hud.setCharacterSize(16); hud.setFillColor({60,60,60}); hud.setPosition(10,10);}
  auto up=[&](){if(font.getInfo().family=="")return; std::ostringstream o;o<<"[LMB] draw  [S] png  [Ctrl+S] save  [Ctrl+O] open\n[U/Ctrl+Z] undo  [R/Ctrl+Y] redo  [E] clear  [C] color  [X] eraser  [ [ / ] ] size\n"
    <<"Brush:"<<(int)radius<<"  Color#"<<ci<<"  Eraser:"<<(eraser?"on":"off")<<"  Strokes:"<<strokes.size()<<"  Redo:"<<redo.size(); hud.setString(o.str());}; up();
  while(win.isOpen()){sf::Event e; while(win.pollEvent(e)){ if(e.type==sf::Event::Closed) win.close();
    if(e.type==sf::Event::KeyPressed){ bool ctrl=sf::Keyboard::isKeyPressed(sf::Keyboard::LControl)||sf::Keyboard::isKeyPressed(sf::Keyboard::RControl);
      if(e.key.code==sf::Keyboard::Escape) win.close();
      else if(e.key.code==sf::Keyboard::S && !ctrl) savePNG(strokes,BG,win.getSize());
      else if(e.key.code==sf::Keyboard::S && ctrl) saveJSON(strokes,"session.json");
      else if(e.key.code==sf::Keyboard::O && ctrl){std::vector<Stroke> ld; if(loadJSON(ld,"session.json")){strokes=std::move(ld); redo.clear();}}
      else if(e.key.code==sf::Keyboard::U || (ctrl && e.key.code==sf::Keyboard::Z)){ if(!strokes.empty()){redo.push_back(std::move(strokes.back())); strokes.pop_back();}}
      else if(e.key.code==sf::Keyboard::R || (ctrl && e.key.code==sf::Keyboard::Y)){ if(!redo.empty()){strokes.push_back(std::move(redo.back())); redo.pop_back();}}
      else if(e.key.code==sf::Keyboard::E){strokes.clear(); redo.clear();}
      else if(e.key.code==sf::Keyboard::C){ci=(ci+1)%pal.size(); color=pal[ci];}
      else if(e.key.code==sf::Keyboard::X){eraser=!eraser;}
      else if(e.key.code==sf::Keyboard::LBracket){radius=std::max(1.f,radius-1.f);}
      else if(e.key.code==sf::Keyboard::RBracket){radius=std::min(64.f,radius+1.f);} up();
    }
    if(e.type==sf::Event::MouseButtonPressed && e.mouseButton.button==sf::Mouse::Left){drawing=true; cur=Stroke{}; cur.color=color; cur.radius=radius; cur.eraser=eraser; cur.points.push_back({(float)e.mouseButton.x,(float)e.mouseButton.y});}
    if(e.type==sf::Event::MouseButtonReleased && e.mouseButton.button==sf::Mouse::Left){ if(drawing){drawing=false; if(!cur.points.empty()){strokes.push_back(std::move(cur)); cur=Stroke{}; redo.clear(); up();}}}
    if(e.type==sf::Event::MouseMoved && drawing){sf::Vector2f p{(float)e.mouseMove.x,(float)e.mouseMove.y}; if(cur.points.empty()||dist(cur.points.back(),p)>=minStep) cur.points.push_back(p);}
  }
  win.clear(BG); for(auto& s:strokes) drawStroke(win,s,BG); if(drawing && !cur.points.empty()) drawStroke(win,cur,BG); if(font.getInfo().family!="") win.draw(hud); win.display(); }
  return 0;
}
