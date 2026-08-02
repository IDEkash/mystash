_G.core = {
	formspec_escape = function(s) return s end
}
dofile("builtin/common/modern_ui.lua")

describe("modern_ui", function()
	it("run_self_tests works successfully", function()
		assert.equal(true, modern_ui.run_self_tests())
	end)

	it("correctly creates parent-child widget tree", function()
		local tree = modern_ui.build(function()
			return window {
				title = "Main Menu",
				column {
					spacing = 10,
					label { text = "Welcome" },
					button { text = "Play" }
				}
			}
		end)

		assert.equal("window", tree.type)
		assert.equal("Main Menu", tree:get_property("title"))
		assert.equal(1, #tree.children)

		local col = tree.children[1]
		assert.equal("column", col.type)
		assert.equal(10, col:get_property("spacing"))
		assert.equal(2, #col.children)

		assert.equal("label", col.children[1].type)
		assert.equal("Welcome", col.children[1]:get_property("text"))

		assert.equal("button", col.children[2].type)
		assert.equal("Play", col.children[2]:get_property("text"))
	end)

	it("handles auto and percentage sizing layout", function()
		local tree = modern_ui.build(function()
			return window {
				width = 10,
				height = 10,
				column {
					width = "100%",
					height = "50%",
					label { height = 2, text = "Hello" },
					button { flex = 1, text = "OK" }
				}
			}
		end)

		tree:compute_layout(10, 10, 0, 0)

		local col = tree.children[1]
		assert.equal(10, col.width)
		assert.equal(5, col.height)

		local lbl = col.children[1]
		local btn = col.children[2]

		-- lbl height is fixed at 2
		assert.equal(2, lbl.height)
		-- btn has flex = 1, should take the remaining height of 3 (col.height 5 - lbl height 2)
		assert.equal(3, btn.height)
	end)

	it("applies theme and resolved styles", function()
		local tree = modern_ui.build(function()
			return window {
				button { style = "custom_btn", text = "Click" }
			}
		end)

		modern_ui.register_style("custom_btn", {
			background = "#FFAA00",
			hover = { background = "#FFCC00" }
		})

		modern_ui.set_theme("dark")

		local btn = tree.children[1]
		local style = btn:resolve_style()

		assert.equal("#FFAA00", style.background)

		-- Simulated hover state
		btn.hovered = true
		local hover_style = btn:resolve_style()
		assert.equal("#FFCC00", hover_style.background)
	end)

	it("supports custom widgets", function()
		modern_ui.register_widget("CustomCard", function(props)
			return modern_ui.build(function()
				return panel {
					styles = { background = props.bg or "#FFFFFF" },
					label { text = props.title or "Untitled" }
				}
			end)
		end)

		local tree = modern_ui.build(function()
			return window {
				CustomCard { title = "My Card", bg = "#FF0000" }
			end
		end)

		assert.equal("window", tree.type)
		local card = tree.children[1]
		assert.equal("panel", card.type)
		assert.equal("#FF0000", card:resolve_style().background)

		local lbl = card.children[1]
		assert.equal("label", lbl.type)
		assert.equal("My Card", lbl:get_property("text"))
	end)

	it("supports interactive reactive data binding", function()
		local state = modern_ui.reactive({ count = 0 })

		local tree = modern_ui.build(function()
			return window {
				label {
					text = state:bind("count", function(val)
						return "Count: " .. tostring(val)
					end)
				}
			end)
		end)

		local lbl = tree.children[1]
		assert.equal("Count: 0", lbl:get_property("text"))

		state.count = 42
		assert.equal("Count: 42", lbl:get_property("text"))
	end)
end)
