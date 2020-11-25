var view_id = 0;
var pixels;
var mapped_buffer;

function CallHandler(id, command, info)
{
    Module.ccall('PostJsMessage', null, ['number', 'string', 'string', 'string'], [view_id, id, command, info]);
}

function LoadWebView(sender, view_info, html, waves)
{
    view_id = sender;
    document.getElementById('image_view').style.display = 'none';
    pixels = null;
    mapped_buffer = null;
    document.getElementById('web_view').style.display = 'block';
    document.getElementById('web_view').onload = function()
    {
        document.getElementById('web_view').contentWindow.CallHandler = CallHandler;
        LoadView(view_info, UTF8ToString(waves));
        CallHandler('body', 'ready', '');
    };
    document.getElementById('web_view').data = 'assets/html/' + UTF8ToString(html) + '.htm';
}

function LoadImageView(sender, view_info, image_width, waves)
{
    view_id = sender;
    document.getElementById('web_view').style.display = 'none';
    document.getElementById('web_view').data = '';
    document.getElementById('image_view').style.display = 'block';
    var image_height = Math.floor(image_width *
        document.getElementById('image_view').offsetHeight / 
        document.getElementById('image_view').offsetWidth);
    document.getElementById('image_view').width = image_width;
    document.getElementById('image_view').height = image_height;
    var ctx = document.getElementById('image_view').getContext("2d");
    pixels = ctx.createImageData(image_width, image_height);
    var pixelsData = Module.ccall('CreatePixels', null, ['number', 'number'], [image_width, image_height]);    
    mapped_buffer = new Uint8ClampedArray(Module.HEAPU8.buffer, pixelsData, image_width * image_height * 4);
    LoadView(view_info, UTF8ToString(waves));
    CallHandler('body', 'ready', image_width / 10 + ' ' + image_width + ' ' + image_height + ' ' + 33619971);
}

function LoadView(view_info, waves)
{
    if ((view_info & 8) != 0)
    {
        document.getElementById('escape').hidden = false;
    }
    else
    {
        document.getElementById('escape').hidden = true;
    }
    document.getElementById('audios').innerHTML = '';
    if (waves != '')
    {
        var audios = waves.split(' ');
        for (var i = 0; i < audios.length; ++i)
        {
            var audio = document.createElement('audio');
            audio.id = 'audio-' + i;
            audio.src = 'assets/wave/' + audios[i] + '.wav';
            audio.type = 'audio/wave';
            document.getElementById('audios').appendChild(audio);
        }
    }
}

function RefreshImageView()
{
    Module.ccall('LockPixels', null, null, null);
    pixels.data.set(mapped_buffer);
    Module.ccall('UnlockPixels', null, null, null);
    document.getElementById('image_view').getContext("2d").putImageData(pixels, 0, 0);
}

Module['onRuntimeInitialized'] = function()
{
    window.onbeforeunload = function()
    {
        Module.ccall('NeedExit', null, null, null);
    }
};
