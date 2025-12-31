const sleep = msec => new Promise(resolve => setTimeout(resolve, msec));

function MediaControlsElement(mediaElement, id)
{
    var controlId = "-webkit-media-controls-" + id;
    this.element = this.find(internals.shadowRoot(mediaElement).firstChild, controlId);
}

MediaControlsElement.prototype = {
    get: function()
    {
        return this.element;
    },

    rect: function()
    {
        var buttonBoundingRect = this.element.getBoundingClientRect();
        return new Array(buttonBoundingRect.left, buttonBoundingRect.top);    
    },

    size: function()
    {
        var buttonBoundingRect = this.element.getBoundingClientRect();
        return new Array(buttonBoundingRect.width, buttonBoundingRect.height);    
    },

    click: function()
    {
        this.moveTo();
        eventSender.mouseDown();
        eventSender.mouseUp();
    },

    moveTo: function()
    {
        var buttonRect = this.rect();
        var buttonSize = this.size();

        var x = (buttonRect[0] + buttonSize[0] / 2);
        var y = (buttonRect[1] + buttonSize[1] / 2);

        eventSender.mouseMoveTo(x, y);
    },

    dragTo: function(add_x, add_y) {
        var buttonRect = this.rect();
        var buttonSize = this.size();

        var x = buttonRect[0] + buttonSize[0] / 2;
        var y = buttonRect[1] + buttonSize[1] / 2;

        eventSender.mouseMoveTo(x, y);
        eventSender.mouseDown();
        eventSender.leapForward(200);
        eventSender.mouseMoveTo(x + add_x, y + add_y);
        eventSender.mouseUp();
    },

    find: function(first, id)
    {
        for (var element = first; element; element = element.nextSibling) {
            // Not every element in the media controls has a shadow pseudo ID, eg. the
            // text nodes for the time values, so guard against exceptions.
            try {
                if (internals.shadowPseudoId(element) == id)
                    return element;
            } catch (exception) { }
    
            if (element.firstChild) {
                var childElement = this.find(element.firstChild, id);
                if (childElement)
                    return childElement;
            }
        }
    
        return null;
    },
}
