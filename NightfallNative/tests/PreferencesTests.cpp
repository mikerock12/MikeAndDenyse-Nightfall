#include "GamePreferences.h"
#include <chrono>
#include <iostream>
#include <stdexcept>

int main() {
    const auto suffix=std::chrono::steady_clock::now().time_since_epoch().count();
    const auto directory=std::filesystem::temp_directory_path()/("slt-preferences-test-"+std::to_string(suffix));
    auto expect=[](bool ok,const char* message){if(!ok)throw std::runtime_error(message);};
    try {
        expect(std::filesystem::create_directory(directory),"test directory already exists");
        for(bool fighting:{false,true}) {
            auto file=directory/(fighting?"fight.cfg":"platform.cfg");
            SLTPrefs::Preferences one(file,fighting);
            one.value.master=.25f;one.value.music=.4f;one.value.effects=.75f;one.value.voice=.8f;
            one.value.fps=144;one.value.quality=3;one.value.leftHanded=true;one.value.touchSize=1.2f;
            one.value.keys[0]=KEY_Q;one.value.keys[10]=KEY_J;expect(one.Save(),"settings write failed");
            SLTPrefs::Preferences two(file,fighting);
            expect(std::fabs(two.value.master-.25f)<.001f&&two.value.fps==144,"settings were not restored");
            expect(two.Key(0,0)==KEY_Q&&two.Key(1,0)==KEY_J,"player bindings were mixed");
            expect(two.RenderScale()==(fighting?6:4),"Ultra preset does not render at 4K");
            Vector2 original{100,420};auto transformed=two.ControlPoint(original,960,540);auto inverse=two.ControlInput(transformed,960,540);
            expect(std::fabs(inverse.x-original.x)<.01f&&std::fabs(inverse.y-original.y)<.01f,"touch display/input transforms disagree");
            std::ofstream invalid(file);invalid<<"master -20\nmusic 8\nquality 999\nfps 9000\ndeadzone -1\ntouchSize 20\nkey0 9999\n";invalid.close();
            SLTPrefs::Preferences bounded(file,fighting);
            expect(bounded.value.master==0&&bounded.value.music==1&&bounded.value.quality==3&&bounded.value.fps==240,"unsafe settings were accepted");
            expect(bounded.value.deadzone>=.05f&&bounded.value.touchSize<=1.35f,"touch/analog bounds not enforced");
            expect(bounded.Key(0,0)==KEY_A,"invalid keyboard code accepted");
        }
        // Only this test-owned new directory is removed; no application profile is touched.
        std::filesystem::remove_all(directory);
        std::cout<<"PREFERENCES_OK: roundtrip, players, limits, touch inversion, 4K presets\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
    return 0;
}
