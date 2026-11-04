#pragma once
#include "raylib.h"
#include <algorithm>
#include <deque>
#include <filesystem>
#include <sstream>
#include <string>
#include <unordered_map>

// Presentation-only audio: never changes simulation clocks, RNG or network data.
class NativeAudio {
    struct Track { Music stream{}; unsigned char* bytes=nullptr; bool ready=false; };
    std::unordered_map<std::string,std::string> musicFiles,voiceFiles,sfxFiles;
    std::string root,currentId;
    Track track;
    Sound voice{};
    bool voiceReady=false,suspended=false;
    std::deque<std::string> lines;
    void DropVoice(){if(voiceReady){StopSound(voice);UnloadSound(voice);voiceReady=false;}}
    void DropTrack(){if(track.ready){StopMusicStream(track.stream);UnloadMusicStream(track.stream);}if(track.bytes)UnloadFileData(track.bytes);track={};}
    bool OpenTrack(const std::string& id){
        auto it=musicFiles.find(id);if(it==musicFiles.end())return false;
        DropTrack();int size=0;track.bytes=LoadFileData(it->second.c_str(),&size);
        if(!track.bytes)return false;
        track.stream=LoadMusicStreamFromMemory(".ogg",track.bytes,size);track.ready=IsMusicValid(track.stream);
        if(!track.ready){DropTrack();return false;}
        track.stream.looping=true;PlayMusicStream(track.stream);if(suspended)PauseMusicStream(track.stream);currentId=id;
        TraceLog(LOG_INFO,"NIGHTFALL_MUSIC: %s seconds=%.2f",id.c_str(),GetMusicTimeLength(track.stream));return true;
    }
public:
    bool Init(){
        if(!IsAudioDeviceReady())return false;
#ifdef PLATFORM_ANDROID
        root="audio/";
#else
        for(const auto& candidate:{std::filesystem::path(GetApplicationDirectory())/"Content/audio",std::filesystem::path("Content/audio"),std::filesystem::path("NightfallNative/Content/audio")}){
            if(std::filesystem::is_regular_file(candidate/"bank.txt")){root=candidate.generic_string()+"/";break;}
        }
        if(root.empty())return false;
#endif
        char* index=LoadFileText((root+"bank.txt").c_str());if(!index)return false;
        std::istringstream input(index);std::string line;while(std::getline(input,line)){
            std::istringstream row(line);std::string type,id,file;row>>type>>id>>file;
            if(file.empty()||file.find("..")!=std::string::npos||file.front()=='/'||file.find(':')!=std::string::npos)continue;
            if(type=="music")musicFiles[id]=root+file;else if(type=="voice")voiceFiles[id]=root+file;else if(type=="sfx")sfxFiles[id]=root+file;
        }
        UnloadFileText(index);TraceLog(LOG_INFO,"NIGHTFALL_AUDIO_BANK: music=%d voice=%d",static_cast<int>(musicFiles.size()),static_cast<int>(voiceFiles.size()));return HasMusic();
    }
    bool HasMusic()const{return !musicFiles.empty();}
    bool HasVoices()const{return !voiceFiles.empty();}
    const std::unordered_map<std::string,std::string>& Effects()const{return sfxFiles;}
    void MusicFor(const std::string& id){if(id!=currentId)OpenTrack(id);}
    bool Dialogue(const std::string& boss,const std::string& hero,bool both){
        lines.clear();DropVoice();
        auto add=[&](const std::string& id){if(voiceFiles.count(id))lines.push_back(id);};
        add(hero+"_"+boss);if(both)add((hero=="mike"?"denyse_":"mike_")+boss);add("boss_"+boss);
        return lines.size()==(both?3U:2U);
    }
    void ClearDialogue(){lines.clear();DropVoice();}
    void Update(bool pause,float musicGain,float voiceGain){
        if(pause!=suspended){
            if(track.ready){if(pause)PauseMusicStream(track.stream);else ResumeMusicStream(track.stream);}
            if(voiceReady){if(pause)PauseSound(voice);else ResumeSound(voice);}suspended=pause;
        }
        if(track.ready){if(!pause)UpdateMusicStream(track.stream);SetMusicVolume(track.stream,std::clamp(musicGain,0.f,1.f)*(voiceReady||!lines.empty()?.3f:1.f));}
        if(voiceReady)SetSoundVolume(voice,std::clamp(voiceGain,0.f,1.f));
        if(pause)return;
        if(voiceReady&&!IsSoundPlaying(voice))DropVoice();
        if(!voiceReady&&!lines.empty()){
            auto id=lines.front();lines.pop_front();int size=0;auto bytes=LoadFileData(voiceFiles.at(id).c_str(),&size);
            if(bytes){Wave wave=LoadWaveFromMemory(".ogg",bytes,size);UnloadFileData(bytes);if(IsWaveValid(wave)){voice=LoadSoundFromWave(wave);UnloadWave(wave);voiceReady=IsSoundValid(voice);}}
            if(voiceReady){SetSoundVolume(voice,std::clamp(voiceGain,0.f,1.f));PlaySound(voice);TraceLog(LOG_INFO,"NIGHTFALL_VOICE: %s",id.c_str());}
        }
    }
    bool Validate(){
        if(musicFiles.size()!=16||voiceFiles.size()!=51||sfxFiles.size()!=8)return false;
        for(const auto& entry:musicFiles){if(!OpenTrack(entry.first)||GetMusicTimeLength(track.stream)<35)return false;UpdateMusicStream(track.stream);}
        for(const auto& entry:voiceFiles){int n=0;auto bytes=LoadFileData(entry.second.c_str(),&n);if(!bytes)return false;Wave w=LoadWaveFromMemory(".ogg",bytes,n);UnloadFileData(bytes);bool ok=IsWaveValid(w)&&w.frameCount>100;UnloadWave(w);if(!ok)return false;}
        for(const auto& entry:sfxFiles){int n=0;auto bytes=LoadFileData(entry.second.c_str(),&n);if(!bytes)return false;Wave w=LoadWaveFromMemory(".ogg",bytes,n);UnloadFileData(bytes);bool ok=IsWaveValid(w)&&w.frameCount>100;UnloadWave(w);if(!ok)return false;}
        TraceLog(LOG_INFO,"NIGHTFALL_AUDIO_SMOKE_OK music=16 voice=%d",static_cast<int>(voiceFiles.size()));return true;
    }
    void Shutdown(){ClearDialogue();DropTrack();musicFiles.clear();voiceFiles.clear();sfxFiles.clear();}
};
