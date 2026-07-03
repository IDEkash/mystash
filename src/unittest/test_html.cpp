// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "test.h"
#include "util/html.h"

class TestHTML : public TestBase {
public:
	TestHTML() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestHTML"; }

	void runTests(IGameDef *gamedef);

	void testSimpleParse();
	void testStreaming();
	void testSpecialTags();
	void testEntities();
	void testVoidElements();
	void testDeepNesting();
};

static TestHTML g_test_instance;

void TestHTML::runTests(IGameDef *gamedef)
{
	TEST(testSimpleParse);
	TEST(testStreaming);
	TEST(testSpecialTags);
	TEST(testEntities);
	TEST(testVoidElements);
	TEST(testDeepNesting);
}

void TestHTML::testSimpleParse()
{
	auto root = html::parse("<div>Hello <b>world</b></div>");
	UASSERTEQ(size_t, root->children.size(), 1);
	auto div = root->children[0];
	UASSERTEQ(std::string, div->name, "div");
	UASSERTEQ(size_t, div->children.size(), 2);
	UASSERTEQ(std::string, div->children[0]->text, "Hello ");
	UASSERTEQ(std::string, div->children[1]->name, "b");
}

void TestHTML::testStreaming()
{
	html::Parser p;
	p.feed("<div ");
	p.feed("class='test'>");
	p.feed("Hello <!-- comment ");
	p.feed("here --> world");
	p.feed("</div>");
	p.end();

	auto root = p.getRoot();
	UASSERTEQ(size_t, root->children.size(), 1);
	auto div = root->children[0];
	UASSERTEQ(std::string, div->attributes["class"], "test");
	UASSERTEQ(size_t, div->children.size(), 3);
	UASSERTEQ(div->children[0]->type, html::NodeType::TEXT);
	UASSERTEQ(div->children[1]->type, html::NodeType::COMMENT);
	UASSERTEQ(div->children[2]->type, html::NodeType::TEXT);
}

void TestHTML::testSpecialTags()
{
	auto root = html::parse("<script>if (a < b) alert('hi');</script><style>body { color: red; }</style>");
	UASSERTEQ(size_t, root->children.size(), 2);

	auto script = root->children[0];
	UASSERTEQ(std::string, script->name, "script");
	UASSERTEQ(size_t, script->children.size(), 1);
	UASSERTEQ(std::string, script->children[0]->text, "if (a < b) alert('hi');");

	auto style = root->children[1];
	UASSERTEQ(std::string, style->name, "style");
	UASSERTEQ(size_t, style->children.size(), 1);
	UASSERTEQ(std::string, style->children[0]->text, "body { color: red; }");
}

void TestHTML::testEntities()
{
	auto root = html::parse("<p>&lt;Hello &amp; world!&gt;</p>");
	UASSERTEQ(size_t, root->children.size(), 1);
	UASSERTEQ(std::string, root->children[0]->children[0]->text, "<Hello & world!>");
}

void TestHTML::testVoidElements()
{
	auto root = html::parse("Line 1<br>Line 2<img src='test.png'>Line 3");
	UASSERTEQ(size_t, root->children.size(), 5);
	UASSERTEQ(std::string, root->children[1]->name, "br");
	UASSERTEQ(std::string, root->children[3]->name, "img");
}

void TestHTML::testDeepNesting()
{
	std::string html;
	for (int i = 0; i < 100; i++) html += "<div>";
	for (int i = 0; i < 100; i++) html += "</div>";

	auto root = html::parse(html);
	UASSERTEQ(size_t, root->children.size(), 1);

	auto current = root->children[0];
	int depth = 0;
	while (!current->children.empty()) {
		current = current->children[0];
		depth++;
	}
	UASSERTEQ(int, depth, 99);
}
