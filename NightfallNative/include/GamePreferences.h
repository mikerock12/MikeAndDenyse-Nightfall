#pragma once
#include "raylib.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

// Presentation preferences are deliberately separate from the deterministic game state.
namespace SLTPrefs {
#if defined(PLATFORM_ANDROID)
inline constexpr bool Mobile = true;
#else
inline constexpr bool Mobile = false;
#endif
struct Values {
    int quality=1, windowMode=0, resolution=0, fps=60;
    bool vsync=true, integerScale=false, particles=true, shake=true, showFps=false, autoPause=true;
    float master=.85f, music=.65f, effects=.85f, voice=.9f, brightness=1, deadzone=.22f;
    bool touch=Mobile, leftHanded=false, vibration=true;
    float touchSize=1, touchOpacity=.65f, touchInset=0;
    std::array<int,20> keys{};
    std::array<int,10> buttons{};
};
class Preferences {
public:
    Values value;
    bool open=false, changed=false, windowChanged=false;
    std::string status;
    bool musicAvailable=true,voicesAvailable=true;
    Preferences(std::filesystem::path file, bool fighter):path(std::move(file)),fighting(fighter) { musicAvailable=voicesAvailable=fighter; Defaults(); Load(); TraceLog(LOG_INFO,"SLT_PREFS_LOADED: mobile=%d master=%.2f left=%d size=%.2f deadzone=%.3f",Mobile,value.master,value.leftHanded,value.touchSize,value.deadzone); }
    int RenderScale() const { return Mobile ? (value.quality==2?2:1) : (value.quality==3&&fighting?6:value.quality+1); }
    int Key(int player,int action) const { return value.keys[player*10+action]; }
    bool Down(int player,int action) const { return IsKeyDown(Key(player,action)); }
    bool Pressed(int player,int action) const { return IsKeyPressed(Key(player,action)); }
    float EffectVolume() const { return value.effects; }
    Vector2 ControlPoint(Vector2 point,float width,float height) const {
        const float inset=value.touchInset*width;
        point.x+=point.x<width/2?inset:-inset;
        point.y-=value.touchInset*height*.35f;
        if(value.leftHanded)point.x=width-point.x;
        return point;
    }
    Vector2 ControlInput(Vector2 point,float width,float height) const {
        if(value.leftHanded)point.x=width-point.x;
        point.x+=point.x<width/2?-value.touchInset*width:value.touchInset*width;
        point.y+=value.touchInset*height*.35f;
        return point;
    }
    void Show(int category=0) { open=true;tab=std::clamp(category,0,4); selected=0;scroll=0; capture=-1; page=0; previousDown=GetTouchPointCount()>0||IsMouseButtonDown(MOUSE_LEFT_BUTTON); changed=false; TraceLog(LOG_INFO,"SLT_SETTINGS: opened mobile=%d",Mobile); }
    void Close() { open=false; capture=-1; Save(); TraceLog(LOG_INFO,"SLT_SETTINGS: saved quality=%d fps=%d",value.quality,value.fps); }
    void Defaults() {
        value=Values{};
        const int fightKeys[20]={KEY_A,KEY_D,KEY_W,KEY_S,KEY_F,KEY_G,KEY_H,KEY_R,KEY_T,KEY_Y, KEY_LEFT,KEY_RIGHT,KEY_UP,KEY_DOWN,KEY_J,KEY_K,KEY_L,KEY_U,KEY_I,KEY_O};
        const int nightKeys[20]={KEY_A,KEY_D,KEY_S,KEY_SPACE,KEY_J,KEY_F,KEY_G,KEY_R,KEY_T,KEY_Y, KEY_LEFT,KEY_RIGHT,KEY_DOWN,KEY_KP_1,KEY_KP_2,KEY_K,KEY_L,KEY_U,KEY_I,KEY_O};
        std::copy_n(fighting?fightKeys:nightKeys,20,value.keys.begin());
        value.buttons={GAMEPAD_BUTTON_RIGHT_FACE_LEFT,GAMEPAD_BUTTON_RIGHT_FACE_UP,GAMEPAD_BUTTON_RIGHT_FACE_RIGHT,GAMEPAD_BUTTON_RIGHT_FACE_DOWN,GAMEPAD_BUTTON_RIGHT_TRIGGER_1,GAMEPAD_BUTTON_RIGHT_TRIGGER_2,GAMEPAD_BUTTON_RIGHT_TRIGGER_2,GAMEPAD_BUTTON_LEFT_TRIGGER_2,GAMEPAD_BUTTON_RIGHT_FACE_DOWN,GAMEPAD_BUTTON_RIGHT_FACE_LEFT};
    }
    bool Save() {
        std::error_code ec; std::filesystem::create_directories(path.parent_path(),ec);
        std::ofstream out(path); if(!out){status="Nao foi possivel salvar as preferencias";return false;}
        out<<"schema 1\n";
#define SLT_SAVE(field) out<<#field<<' '<<value.field<<'\n';
        SLT_SAVE(quality) SLT_SAVE(windowMode) SLT_SAVE(resolution) SLT_SAVE(fps) SLT_SAVE(vsync) SLT_SAVE(integerScale)
        SLT_SAVE(particles) SLT_SAVE(shake) SLT_SAVE(showFps) SLT_SAVE(autoPause) SLT_SAVE(master) SLT_SAVE(music)
        SLT_SAVE(effects) SLT_SAVE(voice) SLT_SAVE(brightness) SLT_SAVE(deadzone) SLT_SAVE(touch) SLT_SAVE(leftHanded)
        SLT_SAVE(vibration) SLT_SAVE(touchSize) SLT_SAVE(touchOpacity) SLT_SAVE(touchInset)
#undef SLT_SAVE
        for(int i=0;i<20;i++)out<<"key"<<i<<' '<<value.keys[i]<<'\n';
        for(int i=0;i<10;i++)out<<"button"<<i<<' '<<value.buttons[i]<<'\n';
        out.flush();TraceLog(LOG_INFO,"SLT_PREFS_SAVED: master=%.2f left=%d size=%.2f deadzone=%.3f",value.master,value.leftHanded,value.touchSize,value.deadzone);return static_cast<bool>(out);
    }
    void Load() {
        std::ifstream input(path); std::string name;float number;
        while(input>>name>>number) {
            if(!std::isfinite(number))continue;
#define SLT_LOAD(field,low,high) if(name==#field){value.field=static_cast<decltype(value.field)>(std::clamp(number,float(low),float(high)));continue;}
            SLT_LOAD(quality,0,Mobile?2:3) SLT_LOAD(windowMode,0,2) SLT_LOAD(resolution,0,3) SLT_LOAD(fps,30,Mobile?120:240)
            SLT_LOAD(vsync,0,1) SLT_LOAD(integerScale,0,1) SLT_LOAD(particles,0,1) SLT_LOAD(shake,0,1) SLT_LOAD(showFps,0,1) SLT_LOAD(autoPause,0,1)
            SLT_LOAD(master,0,1) SLT_LOAD(music,0,1) SLT_LOAD(effects,0,1) SLT_LOAD(voice,0,1) SLT_LOAD(brightness,.65,1.3) SLT_LOAD(deadzone,.05,.6)
            SLT_LOAD(touch,0,1) SLT_LOAD(leftHanded,0,1) SLT_LOAD(vibration,0,1) SLT_LOAD(touchSize,.75,1.35) SLT_LOAD(touchOpacity,.15,.95) SLT_LOAD(touchInset,0,.065)
#undef SLT_LOAD
            for(int i=0;i<20;i++)if(name=="key"+std::to_string(i)&&number>=32&&number<=348)value.keys[i]=static_cast<int>(number);
            for(int i=0;i<10;i++)if(name=="button"+std::to_string(i)&&number>=1&&number<=17)value.buttons[i]=static_cast<int>(number);
        }
        if(Mobile)value.touch=true;
    }
    void ApplyWindow(bool hidden=false) const {
        if(hidden)return;
        if(!Mobile) {
            if(IsWindowFullscreen())ToggleFullscreen();
            if(IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE))ToggleBorderlessWindowed();
            int monitor=GetCurrentMonitor();const int widths[]={1280,1600,1920,2560};
            int width=std::min(widths[value.resolution],std::max(640,GetMonitorWidth(monitor)-80));
            width=std::min(width,std::max(640,(GetMonitorHeight(monitor)-80)*16/9));
            SetWindowSize(width,width*9/16);
            Vector2 origin=GetMonitorPosition(monitor);SetWindowPosition(static_cast<int>(origin.x+(GetMonitorWidth(monitor)-width)/2),static_cast<int>(origin.y+(GetMonitorHeight(monitor)-width*9/16)/2));
            if(value.windowMode==1)ToggleBorderlessWindowed();
            else if(value.windowMode==2){SetWindowSize(GetMonitorWidth(monitor),GetMonitorHeight(monitor));ToggleFullscreen();}
            if(value.vsync)SetWindowState(FLAG_VSYNC_HINT);else ClearWindowState(FLAG_VSYNC_HINT);
        }
        SetTargetFPS(value.fps);
    }
    void Update() {
        if(!open)return;
        changed=false;windowChanged=false;const float scale=std::min(GetScreenWidth()/960.f,GetScreenHeight()/540.f);
        Vector2 pointer=GetTouchPointCount()?GetTouchPosition(0):GetMousePosition();
        pointer={(pointer.x-(GetScreenWidth()-960*scale)/2)/scale,(pointer.y-(GetScreenHeight()-540*scale)/2)/scale};
        bool down=GetTouchPointCount()>0||IsMouseButtonDown(MOUSE_LEFT_BUTTON);bool tapped=down&&!previousDown;previousDown=down;
        auto hit=[&](Rectangle rect){return tapped&&CheckCollisionPointRec(pointer,rect);};
        if(capture>=0) {
            int key=GetKeyPressed();
            if(key==KEY_ESCAPE||IsKeyPressed(KEY_BACK)){capture=-1;status="Alteracao cancelada";return;}
            if(capturePad) { int button=GetGamepadButtonPressed(); if(button>0){value.buttons[capture]=button;capture=-1;status="Botao atualizado";Save();} }
            else if(key>=32&&key<=348) {
                int index=player*10+capture;for(int i=player*10;i<player*10+10;i++)if(i!=index&&value.keys[i]==key)value.keys[i]=value.keys[index];
                value.keys[index]=key;capture=-1;status="Tecla atualizada";Save();
            }
            return;
        }
        if(IsKeyPressed(KEY_ESCAPE)||IsKeyPressed(KEY_BACK)||hit({772,24,160,42})||IsGamepadButtonPressed(0,GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)){Close();return;}
        for(int i=0;i<5;i++)if(hit({26.f+i*184,88,176,43})){tab=i;selected=0;page=0;scroll=0;status.clear();TraceLog(LOG_INFO,"SLT_SETTINGS_TAB: %d",tab);}
        if(IsKeyPressed(KEY_TAB)||IsGamepadButtonPressed(0,GAMEPAD_BUTTON_RIGHT_TRIGGER_1)){tab=(tab+1)%5;selected=0;page=0;scroll=0;}
        auto rows=Rows();
        if(IsKeyPressed(KEY_DOWN)||IsGamepadButtonPressed(0,GAMEPAD_BUTTON_LEFT_FACE_DOWN))selected=(selected+1)%static_cast<int>(rows.size());
        if(IsKeyPressed(KEY_UP)||IsGamepadButtonPressed(0,GAMEPAD_BUTTON_LEFT_FACE_UP))selected=(selected+static_cast<int>(rows.size())-1)%static_cast<int>(rows.size());
        if(Mobile && (IsKeyPressed(KEY_DOWN)||IsKeyPressed(KEY_UP)||IsGamepadButtonPressed(0,GAMEPAD_BUTTON_LEFT_FACE_DOWN)||IsGamepadButtonPressed(0,GAMEPAD_BUTTON_LEFT_FACE_UP)))scroll=(selected/4)*4;
        if(Mobile && rows.size()>4 && hit({720,475,212,54})){scroll=(scroll+4>=static_cast<int>(rows.size()))?0:scroll+4;selected=scroll;}
        int dir=0;
        if(IsKeyPressed(KEY_LEFT)||IsGamepadButtonPressed(0,GAMEPAD_BUTTON_LEFT_FACE_LEFT))dir=-1;
        if(IsKeyPressed(KEY_RIGHT)||IsKeyPressed(KEY_ENTER)||IsGamepadButtonPressed(0,GAMEPAD_BUTTON_LEFT_FACE_RIGHT)||IsGamepadButtonPressed(0,GAMEPAD_BUTTON_RIGHT_FACE_DOWN))dir=1;
        for(int i=Mobile?scroll:0;i<static_cast<int>(rows.size())&&(!Mobile||i<scroll+4);i++) {
            float y=151.f+(i-(Mobile?scroll:0))*(Mobile?78:40);
            if(hit({Mobile?646.f:676.f,y,Mobile?76.f:54.f,Mobile?68.f:34.f})){selected=i;dir=-1;}
            if(hit({Mobile?844.f:866.f,y,Mobile?76.f:54.f,Mobile?68.f:34.f})||hit({36,y,Mobile?600.f:620.f,Mobile?68.f:34.f})){selected=i;dir=1;}
        }
        if(dir && !ReadOnly(selected)) { Edit(selected,dir);changed=true;windowChanged=tab==0&&(Mobile?selected==1:selected<=3);Save(); }
        if(hit({26,Mobile?475.f:489.f,200,Mobile?54.f:34.f})){Defaults();status="Padroes restaurados";changed=true;windowChanged=true;Save();}
    }
    void Draw() const {
        if(!open)return;
        float scale=std::min(GetScreenWidth()/960.f,GetScreenHeight()/540.f);
        Camera2D camera{};camera.zoom=scale;camera.offset={(GetScreenWidth()-960*scale)/2,(GetScreenHeight()-540*scale)/2};
        DrawRectangle(0,0,GetScreenWidth(),GetScreenHeight(),Color{2,4,12,245});BeginMode2D(camera);
        DrawText("CONFIGURACOES",28,30,31,Color{224,204,255,255});Box({772,24,160,42},"Salvar e voltar");
        const char* tabs[]={"Tela","Graficos","Audio","Controles","Celular"};
        for(int i=0;i<5;i++)Box({26.f+i*184,88,176,43},tabs[i],i==tab);
        auto rows=Rows();for(int i=Mobile?scroll:0;i<static_cast<int>(rows.size())&&(!Mobile||i<scroll+4);i++) {
            float y=151.f+(i-(Mobile?scroll:0))*(Mobile?78:40);DrawRectangle(26,static_cast<int>(y)-2,906,38,i==selected?Color{44,41,65,255}:Color{18,20,32,255});
            DrawText(rows[i].first.c_str(),39,static_cast<int>(y)+(Mobile?24:7),18,RAYWHITE);
            if(!ReadOnly(i))Box({Mobile?646.f:676.f,y,Mobile?76.f:54.f,Mobile?68.f:34.f},"-");DrawText(rows[i].second.c_str(),742,static_cast<int>(y)+(Mobile?25:8),16,Color{155,222,255,255});if(!ReadOnly(i))Box({Mobile?844.f:866.f,y,Mobile?76.f:54.f,Mobile?68.f:34.f},"+");
        }
        Box({26,Mobile?475.f:489.f,200,Mobile?54.f:34.f},"Restaurar padroes");
        if(Mobile&&rows.size()>4)Box({720,475,212,54},"Mais opcoes >");
        const char* hint=Mobile?"Toque em uma opcao ou nos botoes - / +. As preferencias ficam salvas.":"Setas: ajustar  |  Tab: categoria  |  Esc: salvar e voltar  |  Controle: direcional / A / B";
        if(!Mobile||!status.empty())DrawText(status.empty()?hint:status.c_str(),240,Mobile?493:501,Mobile?10:12,LIGHTGRAY);
        if(capture>=0){DrawRectangle(110,211,740,125,Color{8,12,22,250});DrawRectangleLines(110,211,740,125,GOLD);DrawText(capturePad?"Pressione o botao do controle":"Pressione a nova tecla",170,238,28,GOLD);DrawText("Esc ou Voltar cancela. Teclas repetidas sao trocadas.",170,290,17,WHITE);}
        EndMode2D();
    }
private:
    std::filesystem::path path;bool fighting;int tab=0,selected=0,player=0,page=0,scroll=0,capture=-1;bool capturePad=false,previousDown=false;
    bool ReadOnly(int row) const {return (Mobile&&tab==0&&(row==0||row==3)) || (Mobile&&tab==4&&row==0) || (!Mobile&&tab==4&&row==5) || (Mobile&&tab==3&&row==2&&!fighting);}
    static const char* On(bool value){return value?"Ligado":"Desligado";}
    static std::string Percent(float n){return std::to_string(static_cast<int>(std::round(n*100)))+"%";}
    static std::string KeyName(int key){if(key==KEY_SPACE)return "Espaco";if(key>=32&&key<127)return std::string(1,static_cast<char>(key));switch(key){case KEY_SPACE:return "Espaco";case KEY_LEFT:return "Esquerda";case KEY_RIGHT:return "Direita";case KEY_UP:return "Cima";case KEY_DOWN:return "Baixo";default:return "Tecla "+std::to_string(key);}}
    static void Box(Rectangle r,const char* text,bool active=false){DrawRectangleRounded(r,.12f,4,active?Color{68,48,99,255}:Color{32,36,53,255});DrawRectangleLinesEx(r,1,active?GOLD:Color{97,120,150,255});int font=std::min(17,static_cast<int>((r.width-10)/std::max(1,static_cast<int>(std::string(text).size()))*1.7f));font=std::max(10,font);DrawText(text,static_cast<int>(r.x+(r.width-MeasureText(text,font))/2),static_cast<int>(r.y+(r.height-font)/2),font,RAYWHITE);}
    std::vector<std::pair<std::string,std::string>> Rows() const {
        if(tab==0){if(Mobile)return {{"Orientacao","Paisagem"},{"Limite de quadros",std::to_string(value.fps)+" FPS"},{"Escala inteira de pixels",On(value.integerScale)},{"Pausar ao sair do app","Sempre"},{"Mostrar FPS",On(value.showFps)}};const char* modes[]={"Janela","Sem bordas","Tela cheia"};const char* sizes[]={"1280 x 720","1600 x 900","1920 x 1080","2560 x 1440"};return {{"Modo de tela",modes[value.windowMode]},{"Resolucao de janela",sizes[value.resolution]},{"Sincronizacao vertical",On(value.vsync)},{"Limite de quadros",std::to_string(value.fps)+" FPS"},{"Escala inteira de pixels",On(value.integerScale)},{"Pausa ao perder foco",On(value.autoPause)},{"Mostrar FPS",On(value.showFps)}};}
        if(tab==1){const char* pc[]={"Baixa","Equilibrada","Alta","Ultra / 4K"};const char* phone[]={"Economia","Equilibrada","Alta"};return {{"Qualidade",Mobile?phone[value.quality]:pc[value.quality]},{"Resolucao interna",std::to_string(RenderScale())+"x"},{"Efeitos de ambiente",On(value.particles)},{"Tremor de camera",On(value.shake)},{"Brilho",Percent(value.brightness)}};}
        if(tab==2){std::vector<std::pair<std::string,std::string>> rows={{"Volume geral",Percent(value.master)}};if(musicAvailable)rows.push_back({"Musica e ambiente",Percent(value.music)});rows.push_back({"Efeitos de combate",Percent(value.effects)});if(voicesAvailable)rows.push_back({"Vozes",Percent(value.voice)});return rows;}
        if(tab==4)return {{"Controles na tela",Mobile?"Sempre visiveis":On(value.touch)},{"Modo canhoto",On(value.leftHanded)},{"Tamanho dos botoes",Percent(value.touchSize)},{"Opacidade",Percent(value.touchOpacity)},{"Margem lateral",Percent(value.touchInset)},{"Vibracao",Mobile?On(value.vibration):"So no Android"}};
        std::vector<std::pair<std::string,std::string>> rows={{"Jogador",player?"P2":"P1"},{"Zona morta do analogico",Percent(value.deadzone)},{Mobile?"Pagina do controle":"Pagina",fighting?(Mobile?(page?"2 / 2":"1 / 2"):(page<2?"Teclado "+std::to_string(page+1):"Controle "+std::to_string(page-1))):(Mobile?"Controle externo":(page?"Controle":"Teclado"))}};
        const char* fight[]={"Esquerda","Direita","Cima / pulo","Baixo","Golpe leve","Golpe forte","Especial","Defesa","Agarrar","Super"};
        const char* pad[]={"Golpe leve","Golpe forte","Especial","Defesa","Agarrar","Super"};
        const char* night[]={"Esquerda","Direita","Descer","Pular","Atacar"};
        if(Mobile || (!fighting&&page) || (fighting&&page>=2)) {
            int first=fighting?(Mobile?page:page-2)*3:8;
            int count=fighting?3:2;
            for(int i=0;i<count;i++)rows.push_back({std::string("Controle: ")+(fighting?pad[first+i]:(i?"Atacar":"Pular")),"Botao "+std::to_string(value.buttons[first+i])});
        } else {
            int first=fighting?page*5:0;
            for(int i=first;i<first+5;i++)rows.push_back({std::string("Tecla: ")+(fighting?fight[i]:night[i]),KeyName(Key(player,i))});
        }
        return rows;
    }
    void Edit(int row,int dir) {
        auto bump=[&](float& v,float step,float lo=0,float hi=1){v=std::clamp(v+step*dir,lo,hi);};
        auto cycle=[&](int& v,int count){v=(v+dir+count)%count;};
        auto cap=[&](){const int pc[]={30,60,90,120,144,165,240};const int phone[]={30,60,90,120};const int* p=Mobile?phone:pc;int n=Mobile?4:7,index=0;while(index<n-1&&p[index]!=value.fps)index++;index=(index+dir+n)%n;value.fps=p[index];};
        if(tab==0){if(Mobile){if(row==1)cap();if(row==2)value.integerScale=!value.integerScale;if(row==4)value.showFps=!value.showFps;}else{switch(row){case 0:cycle(value.windowMode,3);break;case 1:cycle(value.resolution,4);break;case 2:value.vsync=!value.vsync;break;case 3:cap();break;case 4:value.integerScale=!value.integerScale;break;case 5:value.autoPause=!value.autoPause;break;case 6:value.showFps=!value.showFps;break;}}}
        if(tab==1){switch(row){case 0:cycle(value.quality,Mobile?3:4);value.particles=value.quality>0;break;case 1:cycle(value.quality,Mobile?3:4);break;case 2:value.particles=!value.particles;break;case 3:value.shake=!value.shake;break;case 4:bump(value.brightness,.05f,.65f,1.3f);break;}}
        if(tab==2){std::vector<float*> vols={&value.master};if(musicAvailable)vols.push_back(&value.music);vols.push_back(&value.effects);if(voicesAvailable)vols.push_back(&value.voice);bump(*vols[row],.05f);}
        if(tab==3){if(row==0)player=1-player;else if(row==1)bump(value.deadzone,.025f,.05f,.6f);else if(row==2){page=(page+1)%(fighting?(Mobile?2:4):(Mobile?1:2));}else{capturePad=Mobile||(!fighting&&page)||(fighting&&page>=2);capture=(capturePad?(fighting?(Mobile?page:page-2)*3:8):(fighting?page*5:0))+row-3;status="Aguardando comando";}}
        if(tab==4){switch(row){case 0:if(!Mobile)value.touch=!value.touch;break;case 1:value.leftHanded=!value.leftHanded;break;case 2:bump(value.touchSize,.05f,.75f,1.35f);break;case 3:bump(value.touchOpacity,.05f,.15f,.95f);break;case 4:bump(value.touchInset,.005f,0,.065f);break;case 5:value.vibration=!value.vibration;break;}}
    }
};
}
