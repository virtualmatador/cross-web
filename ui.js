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
};
