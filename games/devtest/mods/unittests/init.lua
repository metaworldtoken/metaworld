unittests = {}

local modpath = metaworld.get_modpath("unittests")
dofile(modpath .. "/random.lua")
dofile(modpath .. "/player.lua")
dofile(modpath .. "/crafting_prepare.lua")
dofile(modpath .. "/crafting.lua")
dofile(modpath .. "/itemdescription.lua")

if metaworld.settings:get_bool("devtest_unittests_autostart", false) then
	unittests.test_random()
	unittests.test_crafting()
	unittests.test_short_desc()
	metaworld.register_on_joinplayer(function(player)
		unittests.test_player(player)
	end)
end

