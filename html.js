function ImageMouseDown(event)
{
    var x = Math.floor(event.clientX * document.getElementById('image_view').width /
        document.getElementById('image_view').offsetWidth);
    var y = Math.floor(event.clientY * document.getElementById('image_view').height /
        document.getElementById('image_view').offsetHeight);
    CallHandler("body", "touch-begin", x + ' ' + y);
}

function ImageMouseMove(event)
{
    var x = Math.floor(event.clientX * document.getElementById('image_view').width /
        document.getElementById('image_view').offsetWidth);
    var y = Math.floor(event.clientY * document.getElementById('image_view').height /
        document.getElementById('image_view').offsetHeight);
    CallHandler("body", "touch-move", x + ' ' + y);
}

function ImageMouseUp(event)
{
    var x = Math.floor(event.clientX * document.getElementById('image_view').width /
        document.getElementById('image_view').offsetWidth);
    var y = Math.floor(event.clientY * document.getElementById('image_view').height /
        document.getElementById('image_view').offsetHeight);
    CallHandler("body", "touch-end", x + ' ' + y);
}

function Escape()
{
    Module.ccall('NeedEscape', null, null, null);
}
