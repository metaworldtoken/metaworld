-- Register stuff with empty definitions to test if metaworld fallback options
-- for these things work properly.

-- The itemstrings are deliberately kept descriptive to keep them easy to
-- recognize.

metaworld.register_node("broken:node_with_empty_definition", {})
metaworld.register_tool("broken:tool_with_empty_definition", {})
metaworld.register_craftitem("broken:craftitem_with_empty_definition", {})

metaworld.register_entity("broken:entity_with_empty_definition", {})
