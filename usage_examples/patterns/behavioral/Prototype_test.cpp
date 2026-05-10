/*
 * =============================================================================
 * Prototype (Gang of Four — Creational; documented here with usage examples)
 * =============================================================================
 *
 * Intent (GoF)
 * ------------
 * Specify the kinds of objects to create using a **prototypical instance**, and
 * create new objects by **copying** that prototype.
 *
 * Instead of a class hierarchy of factories or long parameter lists, you keep a
 * representative object (or a registry of them). New instances are produced
 * with `clone()` (or copy construction behind a virtual hook).
 *
 * Typical structure
 * -----------------
 *   • Prototype        — declares `clone()` (or `copy` operation).
 *   • ConcretePrototype — implements clone, often by copy-constructing itself.
 *   • Client           — asks a prototype to duplicate itself, or looks up a
 *     prototype in a **Prototype Manager** / registry and clones it.
 *
 * Why use it
 * ----------
 *   • **Many preset configurations** — "default invoice", "aggressive monster",
 *     "UI skin" — are easier to store as instances than as constructor flags.
 *
 *   • **Subclass explosion** avoided when variants differ mostly by *data* copied
 *     from a template instance rather than by distinct classes.
 *
 *   • **Runtime** choice of template — load prototypes from disk, user picks one,
 *     clone for each new session.
 *
 *   • Integration with **Abstract Factory** or **Builder**: clone a baseline,
 *     then mutate or wrap.
 *
 * Shallow copy vs deep copy
 * -------------------------
 *   • **Shallow** clone shares pointers to subobjects — cheap, but mutations may
 *     alias unexpectedly.
 *
 *   • **Deep** clone duplicates owned subgraphs (vectors, nested heap objects).
 *     Implement `clone()` explicitly when the default copy is wrong.
 *
 * In C++, "the rule of zero" types with value semantics often get correct deep
 * copy from the compiler; types with raw owning pointers need the Rule of Five
 * or smart pointers so `clone()` stays safe.
 *
 * C++ implementation sketch
 * -------------------------
 *
 *   class Shape {
 *    public:
 *     virtual ~Shape() = default;
 *     virtual std::unique_ptr<Shape> clone() const = 0;
 *   };
 *
 *   class Circle : public Shape {
 *    public:
 *     std::unique_ptr<Shape> clone() const override {
 *       return std::make_unique<Circle>(*this); // uses Circle copy ctor
 *     }
 *   };
 *
 * Returning `std::unique_ptr<Shape>` keeps ownership clear and avoids slicing
 * when clients only know the abstract type.
 *
 * Covariant return types
 * ----------------------
 * A derived `clone()` may return `std::unique_ptr<Circle>` only if the base
 * returns a pointer type that allows covariance — `unique_ptr` is **not**
 * covariant in C++. The usual pattern is `unique_ptr<Shape>` everywhere, or a
 * custom `Shape*`/`shared_ptr` design with covariant raw pointers (less ideal).
 *
 * Prototype registry (manager)
 * ----------------------------
 * Map string / enum → prototype instance. `create("orc")` does
 * `prototypes_.at("orc")->clone()`. Register once at startup; clones are cheap
 * copies from the master template.
 *
 * Prototype vs Factory Method
 * ----------------------------
 *   • **Factory Method** — create fresh objects knowing the *class*; subclass
 *     supplies the concrete type.
 *
 *   • **Prototype** — create by *duplicating an instance*; the prototype
 *     encodes the full starting state, including data you might not want in ctor
 *     parameters.
 *
 * Prototype vs copying in application code
 * ----------------------------------------
 *   • A **public copy ctor** alone is not the Prototype *pattern* until you
 *     route creation through polymorphic `clone()` or a registry of templates.
 *
 * Pitfalls
 * --------
 *   • **Slicing** — copying through base without `virtual clone()` loses derived
 *     state.
 *
 *   • **Partial clone** — forgetting to copy a new field when the type evolves.
 *
 *   • **Shared mutable state** after shallow clone — document sharing or deep-copy.
 *
 *   • **Self-referential structures** — graphs may need custom duplication logic.
 *
 * Commented micro-examples (mental model)
 * ---------------------------------------
 *
 *   // Clone then tweak (template + delta):
 *   auto enemy = registry.create("goblin");
 *   enemy->set_level(player_level + 2);
 *
 *   // Deep-copy a document tree:
 *   std::unique_ptr<Node> copy = root->clone(); // each node clones children
 *
 * Testing
 * -------
 *   • Assert `clone()` produces equal value but distinct address.
 *   • Polymorphic: `Shape& ref = circle; auto c = ref.clone();` preserves type
 *     behavior via virtual dispatch.
 *
 * =============================================================================
 */

#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace usage_examples::patterns::behavioral {

	// -----------------------------------------------------------------
	// Abstract prototype — all shapes duplicate through clone().
	// -----------------------------------------------------------------
	struct Shape
	{
		virtual ~Shape() = default;

		[[nodiscard]] virtual std::unique_ptr<Shape> clone() const = 0;
		[[nodiscard]] virtual std::string tag() const = 0;
	};

	struct Circle final : Shape
	{
		int radius{};

		explicit Circle(int r) : radius(r) {}

		[[nodiscard]] std::unique_ptr<Shape> clone() const override
		{
			return std::make_unique<Circle>(*this);
		}

		[[nodiscard]] std::string tag() const override { return "circle"; }
	};

	struct Rectangle final : Shape
	{
		int width{};
		int height{};

		Rectangle(int w, int h) : width(w), height(h) {}

		[[nodiscard]] std::unique_ptr<Shape> clone() const override
		{
			return std::make_unique<Rectangle>(*this);
		}

		[[nodiscard]] std::string tag() const override { return "rectangle"; }
	};

	// -----------------------------------------------------------------
	// Prototype with value-owned subgraph — deep copy via compiler-generated
	// copy of std::vector (illustrates "deep enough" for many DTOs).
	// -----------------------------------------------------------------
	struct PolylineShape final : Shape
	{
		std::vector<std::pair<int, int>> points;

		PolylineShape() = default;

		[[nodiscard]] std::unique_ptr<Shape> clone() const override
		{
			return std::make_unique<PolylineShape>(*this);
		}

		[[nodiscard]] std::string tag() const override { return "polyline"; }
	};

	// -----------------------------------------------------------------
	// Prototype manager — register template instances, create by key.
	// -----------------------------------------------------------------
	class ShapePrototypeRegistry
	{
	public:
		void register_prototype(std::string key, std::unique_ptr<Shape> prototype)
		{
			prototypes_.insert_or_assign(std::move(key), std::move(prototype));
		}

		[[nodiscard]] std::unique_ptr<Shape> create(const std::string& key) const
		{
			const auto it = prototypes_.find(key);
			if (it == prototypes_.end()) throw std::out_of_range("unknown prototype: " + key);
			return it->second->clone();
		}

	private:
		std::unordered_map<std::string, std::unique_ptr<Shape>> prototypes_;
	};

	inline ShapePrototypeRegistry make_demo_shape_registry()
	{
		ShapePrototypeRegistry reg;
		reg.register_prototype("unit-circle", std::make_unique<Circle>(1));
		reg.register_prototype("tile", std::make_unique<Rectangle>(16, 16));
		auto line = std::make_unique<PolylineShape>();
		line->points = { { 0, 0 }, { 1, 2 }, { 3, 3 } };
		reg.register_prototype("zigzag", std::move(line));
		return reg;
	}

} // namespace usage_examples::patterns::behavioral

namespace {

	using usage_examples::patterns::behavioral::Circle;
	using usage_examples::patterns::behavioral::make_demo_shape_registry;
	using usage_examples::patterns::behavioral::PolylineShape;
	using usage_examples::patterns::behavioral::Rectangle;
	using usage_examples::patterns::behavioral::Shape;
	using usage_examples::patterns::behavioral::ShapePrototypeRegistry;

	TEST(PrototypeUsageExamples, CloneCopiesStateWithDistinctIdentity)
	{
		const Circle original(5);
		const auto copy = original.clone();
		ASSERT_TRUE(copy);
		const auto* as_circle = dynamic_cast<Circle*>(copy.get());
		ASSERT_NE(as_circle, nullptr);
		EXPECT_NE(as_circle, &original);
		EXPECT_EQ(as_circle->radius, original.radius);
	}

	TEST(PrototypeUsageExamples, PolymorphicClonePreservesDynamicBehavior)
	{
		const Rectangle rect(10, 20);
		const Shape& ref = rect;
		const auto duplicate = ref.clone();
		ASSERT_TRUE(duplicate);
		EXPECT_EQ(duplicate->tag(), "rectangle");
		const auto* r = dynamic_cast<Rectangle*>(duplicate.get());
		ASSERT_NE(r, nullptr);
		EXPECT_EQ(r->width, 10);
		EXPECT_EQ(r->height, 20);
	}

	TEST(PrototypeUsageExamples, PolylineCloneDuplicatesPointVector)
	{
		PolylineShape path;
		path.points = { { 0, 0 }, { 5, 5 } };
		const auto copy = path.clone();
		auto* p = dynamic_cast<PolylineShape*>(copy.get());
		ASSERT_NE(p, nullptr);
		EXPECT_EQ(p->points.size(), 2u);
		p->points[0].first = 99;
		EXPECT_EQ(path.points[0].first, 0);
	}

	TEST(PrototypeUsageExamples, RegistryCreatesIndependentClonesFromTemplates)
	{
		const auto reg = make_demo_shape_registry();
		auto a = reg.create("unit-circle");
		auto b = reg.create("unit-circle");
		ASSERT_TRUE(a && b);
		auto* ca = dynamic_cast<Circle*>(a.get());
		const auto* cb = dynamic_cast<Circle*>(b.get());
		ASSERT_NE(ca, nullptr);
		ASSERT_NE(cb, nullptr);
		EXPECT_NE(ca, cb);
		EXPECT_EQ(ca->radius, 1);
		EXPECT_EQ(cb->radius, 1);
		ca->radius = 42;
		EXPECT_EQ(cb->radius, 1);
	}

	TEST(PrototypeUsageExamples, UnknownRegistryKeyThrows)
	{
		ShapePrototypeRegistry empty;
		EXPECT_THROW((void)empty.create("missing"), std::out_of_range);
	}

} // namespace

/*
 * Further exercises
 * -----------------
 * 1. Add a `Monster` hierarchy where `clone()` deep-copies `std::vector<Loot*>` by
 *    cloning each `Loot` — or replace with `vector<unique_ptr<Loot>>` and a
 *    custom recursive clone.
 * 2. Combine with **Object Pool**: keep one prototype per type, clone into
 *    pooled buffers for hot paths.
 * 3. Serialize prototypes (JSON) and rebuild — "prototype" becomes data on disk.
 */
