-- Test metaworld.clear_craft function
local function test_clear_craft()
	metaworld.log("info", "[unittests] Testing metaworld.clear_craft")
	-- Clearing by output
	metaworld.register_craft({
		output = "foo",
		recipe = {{"bar"}}
	})
	metaworld.register_craft({
		output = "foo 4",
		recipe = {{"foo", "bar"}}
	})
	assert(#metaworld.get_all_craft_recipes("foo") == 2)
	metaworld.clear_craft({output="foo"})
	assert(metaworld.get_all_craft_recipes("foo") == nil)
	-- Clearing by input
	metaworld.register_craft({
		output = "foo 4",
		recipe = {{"foo", "bar"}}
	})
	assert(#metaworld.get_all_craft_recipes("foo") == 1)
	metaworld.clear_craft({recipe={{"foo", "bar"}}})
	assert(metaworld.get_all_craft_recipes("foo") == nil)
end

-- Test metaworld.get_craft_result function
local function test_get_craft_result()
	metaworld.log("info", "[unittests] Testing metaworld.get_craft_result")

	-- normal
	local input = {
		method = "normal",
		width = 2,
		items = {"", "unittests:coal_lump", "", "unittests:stick"}
	}
	metaworld.log("info", "[unittests] torch crafting input: "..dump(input))
	local output, decremented_input = metaworld.get_craft_result(input)
	metaworld.log("info", "[unittests] torch crafting output: "..dump(output))
	metaworld.log("info", "[unittests] torch crafting decremented input: "..dump(decremented_input))
	assert(output.item)
	metaworld.log("info", "[unittests] torch crafting output.item:to_table(): "..dump(output.item:to_table()))
	assert(output.item:get_name() == "unittests:torch")
	assert(output.item:get_count() == 4)

	-- fuel
	input = {
		method = "fuel",
		width = 1,
		items = {"unittests:coal_lump"}
	}
	metaworld.log("info", "[unittests] coal fuel input: "..dump(input))
	output, decremented_input = metaworld.get_craft_result(input)
	metaworld.log("info", "[unittests] coal fuel output: "..dump(output))
	metaworld.log("info", "[unittests] coal fuel decremented input: "..dump(decremented_input))
	assert(output.time)
	assert(output.time > 0)

	-- cooking
	input = {
		method = "cooking",
		width = 1,
		items = {"unittests:iron_lump"}
	}
	metaworld.log("info", "[unittests] iron lump cooking input: "..dump(output))
	output, decremented_input = metaworld.get_craft_result(input)
	metaworld.log("info", "[unittests] iron lump cooking output: "..dump(output))
	metaworld.log("info", "[unittests] iron lump cooking decremented input: "..dump(decremented_input))
	assert(output.time)
	assert(output.time > 0)
	assert(output.item)
	metaworld.log("info", "[unittests] iron lump cooking output.item:to_table(): "..dump(output.item:to_table()))
	assert(output.item:get_name() == "unittests:steel_ingot")
	assert(output.item:get_count() == 1)

	-- tool repair (repairable)
	input = {
		method = "normal",
		width = 2,
		-- Using a wear of 60000
		items = {"unittests:repairable_tool 1 60000", "unittests:repairable_tool 1 60000"}
	}
	metaworld.log("info", "[unittests] repairable tool crafting input: "..dump(input))
	output, decremented_input = metaworld.get_craft_result(input)
	metaworld.log("info", "[unittests] repairable tool crafting output: "..dump(output))
	metaworld.log("info", "[unittests] repairable tool crafting decremented input: "..dump(decremented_input))
	assert(output.item)
	metaworld.log("info", "[unittests] repairable tool crafting output.item:to_table(): "..dump(output.item:to_table()))
	assert(output.item:get_name() == "unittests:repairable_tool")
	-- Test the wear value.
	-- See src/craftdef.cpp in metaworld source code for the formula. The formula to calculate
	-- the value 51187 is:
	--    65536 - ((65536-60000)+(65536-60000)) + floor(additonal_wear * 65536 + 0.5) = 51187
	-- where additional_wear = 0.05
	assert(output.item:get_wear() == 51187)
	assert(output.item:get_count() == 1)

	-- failing tool repair (unrepairable)
	input = {
		method = "normal",
		width = 2,
		items = {"unittests:unrepairable_tool 1 60000", "unittests:unrepairable_tool 1 60000"}
	}
	metaworld.log("info", "[unittests] unrepairable tool crafting input: "..dump(input))
	output, decremented_input = metaworld.get_craft_result(input)
	metaworld.log("info", "[unittests] unrepairable tool crafting output: "..dump(output))
	metaworld.log("info", "[unittests] unrepairable tool crafting decremented input: "..dump(decremented_input))
	assert(output.item)
	metaworld.log("info", "[unittests] unrepairable tool crafting output.item:to_table(): "..dump(output.item:to_table()))
	-- unrepairable tool must not yield any output
	assert(output.item:get_name() == "")

end

function unittests.test_crafting()
	test_clear_craft()
	test_get_craft_result()
	metaworld.log("action", "[unittests] Crafting tests passed!")
	return true
end

