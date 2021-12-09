--
-- Mod channels experimental handlers
--
local mod_channel = metaworld.mod_channel_join("experimental_preview")

metaworld.register_on_modchannel_message(function(channel, sender, message)
	metaworld.log("action", "[modchannels] Server received message `" .. message
			.. "` on channel `" .. channel .. "` from sender `" .. sender .. "`")

	if mod_channel:is_writeable() then
		mod_channel:send_all("experimental answers to preview")
		mod_channel:leave()
	end
end)
