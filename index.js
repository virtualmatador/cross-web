var view_id = 0;

function CallHandler(id, command, info)
{
    Module.ccall('HandleAsync', null, ['number', 'string', 'string', 'string'], [view_id, id, command, info]);
}

Module['onRuntimeInitialized'] = function()
{
    Module._Begin();
    Module._Create();
    Module._Start();
}

function LoadWebView(sender, view_info, html, waves)
{
    view_id = sender;
    document.getElementById('web_view').onload = function()
    {
        CallHandler('body', 'ready', '');
    };
    document.getElementById('web_view').src = 'assets/html/' + UTF8ToString(html) + '.htm';
}
