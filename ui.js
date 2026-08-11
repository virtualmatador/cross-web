var view_id_ = 0;
var pixels_ = [];

function CallHandler(id, command, info)
{
    Module.ccall('PostJsMessage', null, ['number', 'string', 'string', 'string'], [view_id_, id, command, info]);
}

Module['onRuntimeInitialized'] = function()
{
    window.addEventListener('pagehide', function(event)
    {
        Module.ccall(event.persisted ? 'NeedStop' : 'NeedExit', null, null, null);
    });
    window.addEventListener('pageshow', function(event)
    {
        if (event.persisted)
        {
            Module.ccall('NeedStart', null, null, null);
        }
    });
    document.addEventListener('visibilitychange', function()
    {
        if (document.visibilityState === 'visible')
        {
            Module.ccall('NeedStart', null, null, null);
        }
        else
        {
            Module.ccall('NeedStop', null, null, null);
        }
    });
    if (document.visibilityState !== 'visible')
    {
        Module.ccall('NeedStop', null, null, null);
    }
};
