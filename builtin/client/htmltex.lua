-- Luanti: htmltex module

htmltex = {}

local streaming_views = {}

function htmltex.render(htmlview_id, opts)
    opts = opts or {}
    local tex_name = opts.tex_name or ("dyntex:" .. htmlview_id)

    htmlview.capture(htmlview_id, {
        width = opts.width,
        height = opts.height,
        tex_name = tex_name,
    })

    return tex_name
end

function htmltex.stream(htmlview_id, opts)
    opts = opts or {}
    local tex_name = opts.tex_name or ("dyntex:" .. htmlview_id)
    local interval = opts.interval or 1.0

    if streaming_views[htmlview_id] then
        streaming_views[htmlview_id].stop = true
    end

    local state = { stop = false }
    streaming_views[htmlview_id] = state

    local function step()
        if state.stop then return end

        htmlview.capture(htmlview_id, {
            width = opts.width,
            height = opts.height,
            tex_name = tex_name,
        })

        core.after(interval, step)
    end

    step()

    return tex_name
end

function htmltex.stop_stream(htmlview_id)
    if streaming_views[htmlview_id] then
        streaming_views[htmlview_id].stop = true
        streaming_views[htmlview_id] = nil
    end
end
