#include <sstream>

#include "emmintrin.h"

#include "../core/src/interface.h"

#include "../core/src/bridge.h"

void bridge::NeedRestart()
{
    interface::Restart();
}

void bridge::LoadWebView(const std::int32_t sender, const std::int32_t view_info, const char *html, const char* waves)
{
    EM_ASM(
    {
        LoadWebView($0, $1, $2, $3);
    }, sender, view_info, html, waves);
}

void bridge::LoadImageView(const std::int32_t sender, const std::int32_t view_info, const std::int32_t image_width, const char* waves)
{
    EM_ASM(
    {
        LoadImageView($0, $1, $2, $3);
    }, sender, view_info, image_width, waves);
}

std::uint32_t* bridge::GetPixels()
{
    return nullptr;
}

void bridge::ReleasePixels(std::uint32_t* const pixels)
{
}

void bridge::RefreshImageView()
{
}

void bridge::CallFunction(const char* function)
{
    std::ostringstream js;
    js << "document.getElementById('web_view').contentWindow.eval('" <<
        function << "');";
    emscripten_run_script(js.str().c_str());
}

std::string bridge::GetAsset(const char* key)
{
    return "";
}

std::string bridge::GetPreference(const char* key)
{
    return "";
}

void bridge::SetPreference(const char* key, const char* value)
{
}

void bridge::PostThreadMessage(const std::int32_t sender, const char* id, const char* command, const char* info)
{
}

void bridge::AddParam(const char* key, const char* value)
{
}

void bridge::PostHttp(const std::int32_t sender, const char* id, const char* command, const char* url)
{
}

void bridge::PlayAudio(const std::int32_t index)
{
}

void bridge::Exit()
{
    EM_ASM(
    {
        location = "about:blank";
    });
}
