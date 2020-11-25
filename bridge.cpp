#include <condition_variable>
#include <mutex>
#include <sstream>
#include <list>
#include <vector>

#include "emscripten.h"

#include "../core/src/interface.h"

#include "../core/src/bridge.h"

std::condition_variable work_condition_;
std::mutex work_mutex_;
bool need_escape_{ false };
bool need_restart_{ false };
bool need_exit_{ false };
struct Message
{
    std::int32_t sender;
    std::string id;
    std::string command;
    std::string info;
};
std::list<Message> messages_;
std::list<std::string> functions_;
std::mutex functions_lock_;
std::vector<uint32_t> pixels_;
std::mutex pixels_lock_;

extern "C"
{
    void PostJsMessage(const std::int32_t sender, const char* id, const char* command, const char* info)
    {
        bridge::PostThreadMessage(sender, id, command, info);
    }

    void NeedEscape()
    {
        {
            std::lock_guard<std::mutex> auto_lock{ work_mutex_ };
            need_escape_ = true;
        }
        work_condition_.notify_one();
    }

    const char* PickFunction()
    {
        std::lock_guard<std::mutex> auto_lock{ functions_lock_ };
        return functions_.front().c_str();
    }

    void PopFunction()
    {
        std::lock_guard<std::mutex> auto_lock{ functions_lock_ };
        functions_.pop_front();
    }

    void* CreatePixels(const std::int32_t image_width, const std::int32_t image_height)
    {
        std::lock_guard<std::mutex> auto_lock{ pixels_lock_ };
        pixels_.resize(image_width * image_height);
        return pixels_.data();
    }

    void LockPixels()
    {
        pixels_lock_.lock();
    }

    void UnlockPixels()
    {
        pixels_lock_.unlock();
    }

    void NeedExit()
    {
        {
            std::lock_guard<std::mutex> auto_lock{ work_mutex_ };
            need_exit_ = true;
        }
        work_condition_.notify_one();
    }
}

void bridge::NeedRestart()
{
    {
        std::lock_guard<std::mutex> auto_lock{ work_mutex_ };
        need_restart_ = true;
    }
    work_condition_.notify_one();
}

void bridge::LoadWebView(const std::int32_t sender, const std::int32_t view_info, const char *html, const char* waves)
{
    MAIN_THREAD_ASYNC_EM_ASM(
    {
        LoadWebView($0, $1, $2, $3);
    }, sender, view_info, html, waves);
}

void bridge::LoadImageView(const std::int32_t sender, const std::int32_t view_info, const std::int32_t image_width, const char* waves)
{
    MAIN_THREAD_ASYNC_EM_ASM(
    {
        LoadImageView($0, $1, $2, $3);
    }, sender, view_info, image_width, waves);
}

std::uint32_t* bridge::GetPixels()
{
    pixels_lock_.lock();
    return pixels_.data();
}

void bridge::ReleasePixels(std::uint32_t* const pixels)
{
    pixels_lock_.unlock();
}

void bridge::RefreshImageView()
{
    MAIN_THREAD_ASYNC_EM_ASM(
    {
        RefreshImageView();
    });
}

void bridge::CallFunction(const char* function)
{
    {
        std::lock_guard<std::mutex> auto_lock{ functions_lock_ };
        functions_.push_back(function);
    }
    MAIN_THREAD_ASYNC_EM_ASM(
    {
        document.getElementById('web_view').contentWindow.eval(
            Module.ccall("PickFunction", 'string', null, null));
        Module.ccall("PopFunction", null, null, null)
    });
}

std::string bridge::GetAsset(const char* key)
{
    return "";
}

std::string bridge::GetPreference(const char* key)
{
    char* value = (char*)MAIN_THREAD_EM_ASM_INT(
    {
        var value = localStorage.getItem(UTF8ToString($0)) || '';
        var buffer = Module._malloc(value.length + 1);
        Module.stringToUTF8(value, buffer, value.length + 1);
        return buffer;
    }, key);
    std::string result{ value };
    free(value);
    return result;
}

void bridge::SetPreference(const char* key, const char* value)
{
    MAIN_THREAD_EM_ASM(
    {
        localStorage.setItem(UTF8ToString($0), UTF8ToString($1));
    }, key, value);
}

void bridge::PostThreadMessage(const std::int32_t sender, const char* id, const char* command, const char* info)
{
    {
        std::lock_guard<std::mutex> auto_lock{ work_mutex_ };
        messages_.push_back({sender, id, command, info});
    }
    work_condition_.notify_one();
}

void bridge::AddParam(const char* key, const char* value)
{
}

void bridge::PostHttp(const std::int32_t sender, const char* id, const char* command, const char* url)
{
}

void bridge::PlayAudio(const std::int32_t index)
{
    MAIN_THREAD_ASYNC_EM_ASM(
    {
        var audio = new Audio(document.getElementById('audio-' + $0).getAttribute('src'));
        audio.play();
    }, index);
}

void bridge::Exit()
{
    MAIN_THREAD_EM_ASM(
    {
        location = "about:blank";
    });
    NeedExit();
}

int main()
{
    interface::Begin();
    interface::Create();
    interface::Start();
    for(;;)
    {
        std::unique_lock<std::mutex> work_locker{ work_mutex_ };
        work_condition_.wait(work_locker, [&]()
        {
            return need_escape_ || need_restart_ || need_exit_ || !messages_.empty();
        });
        if (need_exit_)
        {
            need_exit_ = false;
            break;
        }
        if (need_escape_)
        {
            need_escape_ = false;
            work_locker.unlock();
            interface::Escape();
        }
        else if (need_restart_)
        {
            need_restart_ = false;
            work_locker.unlock();
            interface::Restart();
        }
        else
        {
            while(!messages_.empty())
            {
                Message msg = messages_.front();
                messages_.pop_front();
                work_locker.unlock();
                interface::HandleAsync(msg.sender, msg.id.c_str(), msg.command.c_str(), msg.info.c_str());
                work_locker.lock();
            }   
        }        
    }
    interface::Stop();
    interface::Destroy();
    interface::End();
    return 0;
}
