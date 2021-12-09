--
-- Experimental things
--

experimental = {}

dofile(metaworld.get_modpath("experimental").."/detached.lua")
dofile(metaworld.get_modpath("experimental").."/items.lua")
dofile(metaworld.get_modpath("experimental").."/commands.lua")

function experimental.print_to_everything(msg)
	metaworld.log("action", msg)
	metaworld.chat_send_all(msg)
end

metaworld.log("info", "[experimental] modname="..dump(metaworld.get_current_modname()))
metaworld.log("info", "[experimental] modpath="..dump(metaworld.get_modpath("experimental")))
metaworld.log("info", "[experimental] worldpath="..dump(metaworld.get_worldpath()))


metaworld.register_on_mods_loaded(function()
	metaworld.log("action", "[experimental] on_mods_loaded()")
end)
