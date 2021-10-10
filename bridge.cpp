#include <condition_variable>
#include <mutex>
#include <sstream>
#include <list>
#include <vector>

#include "emscripten.h"

#include "extern/core/src/bridge.h"
#include "extern/core/src/cross.h"
#include "extern/core/src/stage.h"

std::condition_variable work_condition_;
std::mutex work_mutex_;
bool need_start_{ true };
bool need_stop_{ false };
bool need_exit_{ false };
bool started_{ false };

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

extern "C"
{
    void PostJsMessage(const std::int32_t sender, const char* id, const char* command, const char* info)
    {
        {
            std::lock_guard<std::mutex> auto_lock{ work_mutex_ };
            messages_.push_back({sender, id, command, info});
        }
        work_condition_.notify_one();
    }

    void NeedExit()
    {
        {
            std::lock_guard<std::mutex> auto_lock{ work_mutex_ };
            need_exit_ = true;
        }
        work_condition_.notify_one();
    }

    void NeedStart()
    {
        {
            std::lock_guard<std::mutex> auto_lock{ work_mutex_ };
            need_start_ = true;
        }
        work_condition_.notify_one();
    }

    void NeedStop()
    {
        {
            std::lock_guard<std::mutex> auto_lock{ work_mutex_ };
            need_stop_ = true;
        }
        work_condition_.notify_one();
    }

    void NeedEscape()
    {
        MAIN_THREAD_EM_ASM(
        {
            Module.ccall('PostJsMessage', null, ['number', 'string', 'string', 'string'],
                [view_id_, "", "escape", ""]);
        });
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
}

void bridge::NeedRestart()
{
    MAIN_THREAD_ASYNC_EM_ASM(
    {
        Module.ccall('PostJsMessage', null, ['number', 'string', 'string', 'string'],
            [view_id_, "", "restart", ""]);
    });
}

void bridge::LoadView(const std::int32_t sender, const std::int32_t view_info, const char *html)
{
    MAIN_THREAD_ASYNC_EM_ASM(
    {
        view_id_ = $0;
        document.getElementById('web_view').onload = function()
        {
            document.getElementById('web_view').contentWindow.CallHandler = CallHandler;
            document.getElementById('web_view').contentWindow.cross_asset_domain_ = '';
            document.getElementById('web_view').contentWindow.cross_asset_async_ = true;
            document.getElementById('web_view').contentWindow.cross_pointer_type_ = 'mouse';
            document.getElementById('web_view').contentWindow.cross_pointer_upsidedown_ = true;
            setTimeout(function()
            {
                CallHandler('body', 'ready', '');
            }, 0);
        };
        document.getElementById('web_view').data = 'assets/' + UTF8ToString($2) + '.htm';
    }, sender, view_info, html);
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
        Module.ccall("PopFunction", null, null, null);
    });
}

std::string bridge::GetPreference(const char* key)
{
    char* value = (char*)MAIN_THREAD_EM_ASM_INT(
    {
        var value = localStorage.getItem(
            UTF8ToString($0) + '/' + UTF8ToString($1)) || '';
        var buffer = Module._malloc(value.length + 1);
        Module.stringToUTF8(value, buffer, value.length + 1);
        return buffer;
    }, PROJECT_NAME, key);
    std::string result{ value };
    free(value);
    return result;
}

void bridge::SetPreference(const char* key, const char* value)
{
    MAIN_THREAD_EM_ASM(
    {
        localStorage.setItem(
            UTF8ToString($0) + '/' + UTF8ToString($1), UTF8ToString($2));
    }, PROJECT_NAME, key, value);
}

void bridge::AsyncMessage(const std::int32_t sender, const char* id, const char* command, const char* info)
{
    MAIN_THREAD_ASYNC_EM_ASM(
    {
        Module.ccall('PostJsMessage', null, ['number', 'string', 'string', 'string'],
            [$0, UTF8ToString($1), UTF8ToString($2), UTF8ToString($3)]);
    }, sender, id, command, info);
}

void bridge::AddParam(const char* key, const char* value)
{
    // TODO
}

void bridge::PostHttp(const std::int32_t sender, const char* id, const char* command, const char* url)
{
    // TODO
}

void bridge::CreateImage(const char* id, const char* parent)
{
    MAIN_THREAD_EM_ASM(
    {
        var id = UTF8ToString($0);
        var web_view = document.getElementById('web_view').contentWindow;
        var img = document.createElement('canvas');
        img.setAttribute('id', id);
        web_view.document.getElementById(UTF8ToString($1)).appendChild(img);
        delete pixels_[id];
    }, id, parent);
}

void bridge::ResetImage(const std::int32_t sender, const std::int32_t index, const char* id)
{
    std::ostringstream os;
    os << "cross://" << sender << '/' << id << '/' << index;
    cross::FeedUri(os.str().c_str(), [&](const std::vector<unsigned char>& input)
    {
        MAIN_THREAD_EM_ASM(
        {
            var id = UTF8ToString($0);
            var img = document.getElementById('web_view').contentWindow.document.getElementById(id);
            var ctx = img.getContext('2d');
            var bmp_width = $3;
            var bmp_height = $4;
            if (!pixels_[id] ||
                pixels_[id].width != bmp_width ||
                pixels_[id].height != bmp_height)
            {
                img.width = bmp_width;
                img.height = bmp_height;
                pixels_[id] = ctx.createImageData(bmp_width, bmp_height);
            }
            var mapped_buffer = new Uint8ClampedArray(Module.HEAPU8.buffer, $1, $2);
            pixels_[id].data.set(mapped_buffer);
            ctx.putImageData(pixels_[id], 0, 0);
        }, id, input.data() + 54, input.size() - 54,
            *(int32_t*)&input[18], *(int32_t*)&input[22]);
    });
}

void bridge::Exit()
{
    MAIN_THREAD_ASYNC_EM_ASM(
    {
        location = "about:blank";
    });
}

int main()
{
    cross::Begin();
    cross::Create();
    for(;;)
    {
        std::unique_lock<std::mutex> work_locker{ work_mutex_ };
        work_condition_.wait(work_locker, [&]()
        {
            return need_exit_ || need_stop_ || need_start_ || !messages_.empty();
        });
        if (need_exit_)
        {
            work_locker.unlock();
            break;
        }
        else if (need_stop_)
        {
            need_stop_ = false;
            if (started_)
            {
                started_ = false;
                work_locker.unlock();
                cross::Stop();
            }
        }
        else if (need_start_)
        {
            need_start_ = false;
            if (!started_)
            {
                started_ = true;
                work_locker.unlock();
                cross::Start();
            }
        }
        else
        {
            Message msg = messages_.front();
            messages_.pop_front();
            work_locker.unlock();
            if (!msg.id.empty())
            {
                cross::HandleAsync(msg.sender, msg.id.c_str(), msg.command.c_str(), msg.info.c_str());
            }
            else if (msg.sender == core::Stage::index_)
            {
                if (msg.command == "restart")
                {
                    cross::Restart();
                }
                else if (msg.command == "escape")
                {
                    cross::Escape();
                }
            }
        }
    }
    if (started_)
    {
        started_ = false;
        cross::Stop();
    }
    cross::Destroy();
    cross::End();
    return 0;
}
