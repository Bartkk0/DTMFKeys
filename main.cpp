#include <gtkmm.h>
#include <gtkmm/window.h>

#include <iostream>
#include <map>
#include <SDL2/SDL_audio.h>
#include <cmath>

int amplitude = 3000;

enum PhoneState {
    NONE,
    NUMBER,
    DIAL,
    RING,
    BUSY
};

std::vector<std::pair<const std::string, std::pair<int, int>>> num_buttons = {
        {"1", {1207, 697}},
        {"2", {1336, 697}},
        {"3", {1477, 697}},
        {"A", {1633, 697}},

        {"4", {1207, 770}},
        {"5", {1336, 770}},
        {"6", {1477, 770}},
        {"B", {1633, 770}},

        {"7", {1207, 852}},
        {"8", {1336, 852}},
        {"9", {1477, 852}},
        {"C", {1633, 852}},

        {"*", {1209, 941}},
        {"0", {1336, 941}},
        {"#", {1477, 941}},
        {"D", {1633, 941}},
};

std::vector<std::pair<const std::string, PhoneState>> extra_buttons = {
        {"Dial", DIAL},
        {"Ring", RING},
        {"Busy", BUSY},
};

std::map<int,std::pair<int,int>> keybinds = {};

static PhoneState current_state;
static std::pair<int, int> currentKey = {0, 0};

class ButtonSelectDialog : public Gtk::Dialog {
public:
    ButtonSelectDialog(){
        signal_key_press_event().connect([&](GdkEventKey *event) -> bool {
            this->response(event->keyval);
            this->hide();
        });

        label.show();
        get_content_area()->add(label);
    }
protected:
    Gtk::Label label{"Press a key..."};
};

class MainWindow : public Gtk::Window {
public:
    MainWindow() {
        set_border_width(10);

        m_buttonsFlow.set_max_children_per_line(4);
        m_buttonsFlow.set_min_children_per_line(4);
        m_buttonsFlow.set_column_spacing(0);
        m_buttonsFlow.set_row_spacing(0);

        for (const auto &item: num_buttons) {
            m_buttons.push_back(new Gtk::Button(item.first));
            auto &btn = *m_buttons.back();
            btn.set_border_width(0);
            btn.signal_pressed().connect([item] {
                currentKey = item.second;
                current_state = NUMBER;
            });
            btn.signal_released().connect([]  {
                current_state = NONE;
            });
            btn.signal_button_press_event().connect([&](GdkEventButton *event) -> bool{
                if(event->button == 3){
                    int key = this->selectDialog.run();
                    keybinds.emplace(key,item.second);
                }
                return true;
            });
            m_buttonsFlow.add(btn);
        }

        m_mainGrid.add(m_buttonsFlow);

        m_extraButtonsFlow.set_orientation(Gtk::ORIENTATION_VERTICAL);
        for (const auto &item: extra_buttons) {
            m_buttons.push_back(new Gtk::Button(item.first));
            auto &btn = *m_buttons.back();
            btn.signal_pressed().connect([item] {
                current_state = item.second;
            });
            btn.signal_released().connect([item] {
                current_state = NONE;
            });
            m_extraButtonsFlow.add(btn);
        }

        m_mainGrid.add(m_extraButtonsFlow);
        m_mainGrid.set_orientation(Gtk::ORIENTATION_VERTICAL);

        signal_key_press_event().connect([&]( GdkEventKey *event) -> bool{
            if(keybinds.count(event->keyval)){
                current_state = NUMBER;
                currentKey = keybinds.at(event->keyval);
            }
        });

        signal_key_release_event().connect([&]( GdkEventKey *event) -> bool{
            if(keybinds.count(event->keyval) && keybinds.at(event->keyval) == currentKey){
                current_state = NONE;
            }
        });

        m_range.set_value(amplitude);
        m_range.signal_change_value().connect([](Gtk::ScrollType type, double value) -> bool {
            amplitude = value;
        });
        m_mainGrid.attach(m_range,0,1,2,1);

        m_mainGrid.show_all();
        add(m_mainGrid);
    };

    virtual ~MainWindow() {

    };
protected:
    std::vector<Gtk::Button *> m_buttons = {};
    Gtk::FlowBox m_buttonsFlow;
    Gtk::FlowBox m_extraButtonsFlow;
    Gtk::Grid m_mainGrid;
    Gtk::HScale m_range{0,10000,1};
    ButtonSelectDialog selectDialog;
};

void audio_callback(void *user_data, Uint8 *raw_buffer, int bytes) {
    Sint16 *buffer = (Sint16 *) raw_buffer;
    int length = bytes / 2; // 2 bytes per sample for AUDIO_S16SYS
    Uint64 &sample_nr(*(Uint64 *) user_data);

    for (int i = 0; i < length; i++, sample_nr++) {
        double time = (double) sample_nr / (double) 48000;
        switch (current_state) {

            case NONE:
                buffer[i] = 0;
                break;
            case NUMBER:
                buffer[i] = (Sint16) (amplitude * (sin(2.0f * M_PI * (float) currentKey.first * time) +
                                                   sin(2.0f * M_PI * (float) currentKey.second * time)));
                break;
            case DIAL:
                buffer[i] = (Sint16) (amplitude * (sin(2.0f * M_PI * 425 * time)));
                break;
            case RING:
                if (std::fmod(time,5)<1) {
                    buffer[i] = (Sint16) (amplitude * (sin(2.0f * M_PI * 425 * time)));
                } else {
                    buffer[i] = 0;
                }
                break;
            case BUSY:
                if (std::fmod(time,1)<0.5) {
                    buffer[i] = (Sint16) (amplitude * (sin(2.0f * M_PI * 425 * time)));
                } else {
                    buffer[i] = 0;
                }
                break;
        }
    }
}

int main(int argc, char *argv[]) {

    SDL_AudioInit(0);

    SDL_AudioSpec audio_spec;
    audio_spec.freq = 48000;
    audio_spec.format = AUDIO_S16SYS;
    audio_spec.channels = 1;
    audio_spec.samples = 2048;
    audio_spec.callback = audio_callback;
    audio_spec.userdata = malloc(sizeof(Uint64));
    *((Uint64*)audio_spec.userdata) = 0;
    SDL_AudioDeviceID audio_device = SDL_OpenAudioDevice(nullptr, 0, &audio_spec, nullptr, 0);
    std::cout << SDL_GetError() << "\n";

    SDL_PauseAudioDevice(audio_device, 0);

    auto app = Gtk::Application::create(argc, argv, "pl.bartkk.dtmf");

    MainWindow mainWindow;
    return app->run(mainWindow);
}
