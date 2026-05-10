/*
 * =============================================================================
 * Visitor (Gang of Four — Behavioral)
 * =============================================================================
 *
 * Intent (GoF)
 * ------------
 * Represent an **operation** to be performed on the **elements** of an object
 * structure. Visitor lets you define a **new operation** without changing the
 * classes of the elements on which it operates.
 *
 * Element classes expose **`accept(Visitor&)`**, which immediately calls back
 * **`visitor.visit(*this)`** — this **double dispatch** selects the correct
 * overload for the *runtime* concrete element type.
 *
 * Typical structure
 * -----------------
 *   • **Visitor** — declares `visit(ConcreteElementA&)`, `visit(ConcreteElementB&)`, …
 *   • **ConcreteVisitor** — implements those visits; holds accumulated state
 *     (report string, validation errors, rendered output).
 *   • **Element** — declares `accept(Visitor&)`.
 *   • **ConcreteElement** — `accept` forwards to `visitor.visit(*this)`.
 *
 * Trade-off (the “expression problem”)
 * -------------------------------------
 *   • **Easy:** add a **new Visitor** (export to JSON, compute area, lint) —
 *     existing element `.cpp` files stay untouched if `accept` is already there.
 *
 *   • **Hard:** add a **new Element type** — every existing Visitor must gain a
 *     new `visit(NewThing&)` and compile until exhaustive.
 *
 *   • Contrast with **ordinary virtuals** on Element (`virtual double area()`):
 *     new **types** are easy; new **operations** require editing every class.
 *
 * Visitor vs Iterator
 * -------------------
 *   • **Iterator** traverses structure and returns elements; operations live in
 *     the client loop.
 *
 *   • **Visitor** bundles **type-specific logic** in visitor overloads, often
 *     cleaner when behavior varies strongly per concrete type.
 *
 * C++ alternatives
 * ----------------
 *   • **`std::variant<Circle, Rectangle>` + `std::visit`** — closed set of types
 *     in one place; compiler enforces exhaustiveness in a single `visit` lambda
 *     list (C++17). Great when you own all types in one module.
 *
 *   • **`dynamic_cast` chains** — brittle; prefer Visitor or variant.
 *
 *   • **CRTP or templates** — open generics, different trade-offs than classic
 *     Visitor.
 *
 * Practical notes
 * ---------------
 *   • **Forward-declare** element types where possible, but the Visitor’s
 *     `visit(ConcreteElement&)` overloads should be declared only once those
 *     element types are **complete**; otherwise some compilers emit bad vtables.
 *     A common layout is: complete elements + `be_visited_by` declarations, then
 *     the Visitor interface, then out-of-line `be_visited_by` bodies that call
 *     `visitor.visit(*this)`.
 *
 *   • **One-definition rule across TUs** — this repo links many pattern demos
 *     into one `usage_examples` binary. Types such as `Circle` / `Rectangle`
 *     also appear in other examples under `behavioral`, so this file nests its
 *     demo in `visitor_gof` to avoid **ODR violations** and corrupted vtables.
 *
 *   • **Avoid fragile base names on Windows** — SDKs have used names like
 *     `IShape` for COM/graphics-related symbols; a stray macro or typedef can
 *     hijack a token and silently break hierarchies. Prefer domain-specific names
 *     (`ShapeElement`, `DocumentNode`, …).
 *
 *   • **Const visitors** — use `void be_visited_by(ConstShapeVisitor&) const` and
 *     `visit(const Circle&)` when operations are read-only.
 *
 *   • **Windows headers** — Winsock historically `#define accept ...`. If that
 *     macro is visible, a member named `accept` can break calls through base
 *     pointers; use another method name (here: `be_visited_by`) or `#undef accept`.
 *
 *   • **Return values** — visitors often **mutate internal state** instead of
 *     returning from `visit` (void return keeps the pattern simple).
 *
 * Pitfalls
 * --------
 *   • **Null or incomplete graphs** — Visitor walks structure; document who
 *     owns children and when nodes are valid.
 *
 *   • **Cycles** — visiting a cyclic graph needs a visited-set unless you know
 *     the structure is a DAG.
 *
 *   • **Default visits** — a base `visit(ShapeElement&)` rarely helps; overload
 *     resolution targets static types. Optional “catch-all” requires careful design.
 *
 * Testing
 * -------
 *   • Feed a small **fixture tree** to each ConcreteVisitor and assert strings,
 *     counts, or error flags.
 *
 * =============================================================================
 */

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <typeinfo>
#include <vector>

namespace usage_examples::patterns::behavioral::visitor_gof {

	struct Circle;
	struct Rectangle;
	struct ShapeVisitor;

	// -----------------------------------------------------------------
	// Elements — `be_visited_by` is the GoF `accept` operation; we avoid the name
	// `accept` (Winsock macro) and `visit` as a method name on the element.
	//
	// `ShapeVisitor` is only forward-declared here; out-of-line `be_visited_by`
	// definitions call `visitor.visit(*this)` only after `ShapeVisitor` is fully
	// defined. The visitor interface is placed *after* concrete shapes so
	// `visit(Circle&)` / `visit(Rectangle&)` see complete types.
	// -----------------------------------------------------------------
	struct ShapeElement
	{
		virtual ~ShapeElement() = default;
		virtual void be_visited_by(ShapeVisitor& visitor) = 0;
	};

	struct Circle : ShapeElement
	{
		int radius{};

		explicit Circle(int r) : radius(r) {}

		void be_visited_by(ShapeVisitor& visitor) override;
	};

	struct Rectangle : ShapeElement
	{
		int width{};
		int height{};

		Rectangle(int w, int h) : width(w), height(h) {}

		void be_visited_by(ShapeVisitor& visitor) override;
	};

	// -----------------------------------------------------------------
	// Visitor interface — `visit` overloads use complete `Circle` / `Rectangle`.
	// -----------------------------------------------------------------
	struct ShapeVisitor
	{
		virtual ~ShapeVisitor() = default;
		virtual void visit(Circle& c) = 0;
		virtual void visit(Rectangle& r) = 0;
	};

	inline void Circle::be_visited_by(ShapeVisitor& visitor) { visitor.visit(*this); }

	inline void Rectangle::be_visited_by(ShapeVisitor& visitor) { visitor.visit(*this); }

	// -----------------------------------------------------------------
	// Concrete visitors — new operations without editing Circle or Rectangle.
	// -----------------------------------------------------------------
	struct DescribeVisitor : ShapeVisitor
	{
		std::string text;

		void visit(Circle& c) override
		{
			append_separator();
			text += "circle(r=" + std::to_string(c.radius) + ")";
		}

		void visit(Rectangle& r) override
		{
			append_separator();
			text += "rect(" + std::to_string(r.width) + "x" + std::to_string(r.height) + ")";
		}

	private:
		void append_separator()
		{
			if (!text.empty()) text += " | ";
		}
	};

	struct CircleCountVisitor : ShapeVisitor
	{
		int circles = 0;

		void visit(Circle&) override { ++circles; }

		void visit(Rectangle&) override {}
	};

	struct BoundingSumVisitor : ShapeVisitor
	{
		// Crude “complexity” score: circle uses 2*r as pseudo-diameter sum,
		// rectangle uses width+height.
		int sum = 0;

		void visit(Circle& c) override { sum += 2 * c.radius; }

		void visit(Rectangle& r) override { sum += r.width + r.height; }
	};

	inline void apply_visitor(std::vector<std::unique_ptr<ShapeElement>>& shapes, ShapeVisitor& visitor)
	{
		for (auto& shape : shapes) shape->be_visited_by(visitor);
	}

	TEST(VisitorUsageExamples, DirectAcceptDispatchesToConcreteVisitor)
	{
		Circle c(3);
		DescribeVisitor describe;
		c.be_visited_by(describe);
		EXPECT_EQ(describe.text, "circle(r=3)");
	}

	TEST(VisitorUsageExamples, AcceptThroughShapeElementPointerUsesVirtualDispatch)
	{
		Circle c(9);
		ShapeElement* through_base = &c;
		EXPECT_EQ(typeid(*through_base), typeid(Circle));
		DescribeVisitor describe;
		through_base->be_visited_by(describe);
		EXPECT_EQ(describe.text, "circle(r=9)");
	}

	TEST(VisitorUsageExamples, DescribeVisitorAccumulatesPerType)
	{
		std::vector<std::unique_ptr<ShapeElement>> doc;
		doc.push_back(std::make_unique<Circle>(5));
		doc.push_back(std::make_unique<Rectangle>(3, 4));
		doc.push_back(std::make_unique<Circle>(1));

		DescribeVisitor describe;
		apply_visitor(doc, describe);

		EXPECT_EQ(describe.text, "circle(r=5) | rect(3x4) | circle(r=1)");
	}

	TEST(VisitorUsageExamples, CircleCountVisitorIgnoresRectangles)
	{
		std::vector<std::unique_ptr<ShapeElement>> doc;
		doc.push_back(std::make_unique<Rectangle>(1, 1));
		doc.push_back(std::make_unique<Circle>(2));
		doc.push_back(std::make_unique<Circle>(3));

		CircleCountVisitor counter;
		apply_visitor(doc, counter);

		EXPECT_EQ(counter.circles, 2);
	}

	TEST(VisitorUsageExamples, AnotherOperationWithoutChangingShapes)
	{
		std::vector<std::unique_ptr<ShapeElement>> doc;
		doc.push_back(std::make_unique<Circle>(10));
		doc.push_back(std::make_unique<Rectangle>(2, 5));

		BoundingSumVisitor summer;
		apply_visitor(doc, summer);

		EXPECT_EQ(summer.sum, 2 * 10 + 2 + 5);
	}

} // namespace usage_examples::patterns::behavioral::visitor_gof

/*
 * Further exercises
 * -----------------
 * 1. Add `Triangle` to the hierarchy and fix every `ShapeVisitor` — feel the
 *    trade-off of extending the Element side.
 * 2. Reimplement the same shapes as `std::variant<Circle, Rectangle>` and compare
 *    ergonomics of `std::visit` vs classic Visitor.
 * 3. Add a **const** read-only visitor path (`void be_visited_by(ConstShapeVisitor&) const`)
 *    and use it from a `PrintVisitor` that must not mutate nodes.
 */
