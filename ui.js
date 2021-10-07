var view_id_ = 0;
var pixels_ = [];

function CallHandler(id, command, info)
{
    Module.ccall('PostJsMessage', null, ['number', 'string', 'string', 'string'], [view_id_, id, command, info]);
}

Module['onRuntimeInitialized'] = function()
{
    window.onbeforeunload = function()
    {
        Module.ccall('NeedExit', null, null, null);
    };
    document.onvisibilitychange = function()
    {
        if (document.visibilityState === 'visible')
        {
            Module.ccall('NeedStart', null, null, null);
        }
        else
        {
            Module.ccall('NeedStop', null, null, null);
        }
    }; 
};
