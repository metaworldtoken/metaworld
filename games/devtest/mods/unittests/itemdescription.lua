local full_description = "Colorful Pickaxe\nThe best pick."
metaworld.register_tool("unittests:colorful_pick", {
	description = full_description,
	inventory_image = "basetools_mesepick.png",
	tool_capabilities = {
		full_punch_interval = 1.0,
		max_drop_level=3,
		groupcaps={
			cracky={times={[1]=2.0, [2]=1.0, [3]=0.5}, uses=20, maxlevel=3},
			crumbly={times={[1]=2.0, [2]=1.0, [3]=0.5}, uses=20, maxlevel=3},
			snappy={times={[1]=2.0, [2]=1.0, [3]=0.5}, uses=20, maxlevel=3}
		},
		damage_groups = {fleshy=4},
	},
})

metaworld.register_chatcommand("item_description", {
	param = "",
	description = "Show the short and full description of the wielded item.",
	func = function(name)
		local player = metaworld.get_player_by_name(name)
		local item = player:get_wielded_item()
		return true, string.format("short_description: %s\ndescription: %s",
				item:get_short_description(), item:get_description())
	end
})

function unittests.test_short_desc()
	local function get_short_description(item)
		return ItemStack(item):get_short_description()
	end

	local stack = ItemStack("unittests:colorful_pick")
	assert(stack:get_short_description() == "Colorful Pickaxe")
	assert(get_short_description("unittests:colorful_pick") == "Colorful Pickaxe")
	assert(metaworld.registered_items["unittests:colorful_pick"].short_description == nil)
	assert(stack:get_description() == full_description)
	assert(stack:get_description() == metaworld.registered_items["unittests:colorful_pick"].description)

	stack:get_meta():set_string("description", "Hello World")
	assert(stack:get_short_description() == "Hello World")
	assert(stack:get_description() == "Hello World")
	assert(get_short_description(stack) == "Hello World")
	assert(get_short_description("unittests:colorful_pick") == "Colorful Pickaxe")

	stack:get_meta():set_string("short_description", "Foo Bar")
	assert(stack:get_short_description() == "Foo Bar")
	assert(stack:get_description() == "Hello World")

	return true
end
