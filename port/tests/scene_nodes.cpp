// Standalone shared scene-node controls: zig build test-scene_nodes.
// No window or game data; exercise the port's actual animated-mesh fallback and recovered nodes.
#include "ox/scene/ISceneNode.h"
#include "ox/scene/ITriangleSelector.h"
#include "scene/CAnimatedMeshSceneNode.h"
#include "daisy/scene/CDummyTransformationSceneNode.h"
#include "daisy/scene/CTriangleBBSelector.h"
#include <cmath>
#include <cstdio>

#define CHECK(c) do { if (!(c)) { std::fprintf(stderr, "Scene node check failed: %s\n", #c); return 1; } } while (0)
typedef ox::core::CVector3d<float> Vec;
typedef ox::core::CAabbox3d<float> Box;
static int nodesDestroyed = 0, selectorsDestroyed = 0, animatorsDestroyed = 0, animationCalls = 0;
class Node : public ox::scene::ISceneNode
{
public:
    Node(ox::scene::ISceneNode* parent = 0) : ISceneNode(parent, 0) {}
    ~Node() { ++nodesDestroyed; }
    void render() {}
    const Box& getBoundingBox() const { return box; }
private:
    Box box;
};
class Selector : public ox::scene::ITriangleSelector
{
public:
    ~Selector() { ++selectorsDestroyed; }
    int getTriangleCount() const { return 0; }
    void getTriangles(ox::core::CTriangle3d<float>*, int, int& count, const ox::core::CMatrix4*) { count = 0; }
    void getTriangles(ox::core::CTriangle3d<float>*, int, int& count, const Box&, const ox::core::CMatrix4*) { count = 0; }
    void getTriangles(ox::core::CTriangle3d<float>*, int, int& count, const ox::core::CLine3d<float>&,
        const ox::core::CMatrix4*) { count = 0; }
};
class Animator : public ox::scene::ISceneNodeAnimator
{
public:
    ~Animator() { ++animatorsDestroyed; }
    void animateNode(ox::scene::ISceneNode* node, unsigned int time)
    {
        ++animationCalls;
        node->setPosition(Vec((float)time, 0, 0));
    }
};
static bool close(float a, float b) { return std::fabs(a - b) < 0.0001f; }

int main()
{
    Node* root = new Node();
    root->addChild(0);
    CHECK(root->getChildren().begin() == root->getChildren().end());
    root->setPosition(Vec(2, 0, 0));
    Node* child = new Node(root);
    child->setPosition(Vec(3, 0, 0));
    root->OnPreRender();
    CHECK(child->getAbsolutePosition() == Vec(5, 0, 0));
    root->setVisible(false);
    root->setPosition(Vec(10, 0, 0));
    root->OnPreRender();
    CHECK(child->getAbsolutePosition() == Vec(5, 0, 0));
    root->setVisible(true);
    root->OnPreRender();
    CHECK(child->getAbsolutePosition() == Vec(13, 0, 0));

    Node* other = new Node();
    other->setPosition(Vec(40, 0, 0));
    child->setParent(other);
    CHECK(root->getChildren().begin() == root->getChildren().end());
    other->OnPreRender();
    CHECK(child->getAbsolutePosition() == Vec(43, 0, 0));
    Animator* animator = new Animator();
    child->addAnimator(animator);
    CHECK(!animator->drop());
    other->OnPostRender(7);
    CHECK(animationCalls == 1 && child->getAbsolutePosition() == Vec(47, 0, 0));
    child->removeAnimators();
    CHECK(animatorsDestroyed == 1);

    Selector* selector = new Selector();
    child->setTriangleSelector(selector);
    CHECK(!selector->drop()); // The node owns the remaining reference.
    child->setTriangleSelector(child->getTriangleSelector());
    CHECK(selectorsDestroyed == 0 && child->getTriangleSelector() == selector);
    child->setTriangleSelector(0);
    CHECK(selectorsDestroyed == 1 && child->getTriangleSelector() == 0);
    child->setTriangleSelector(0);
    selector = new Selector();
    child->setTriangleSelector(selector);
    CHECK(!selector->drop());

    CHECK(root->getMaterialCount() == 0);
    ox::video::SMaterial* fallback = &root->getMaterial(0);
    CHECK(fallback != 0);
    fallback->Lighting = false;
    daisy::scene::CAnimatedMeshSceneNode* mesh = new daisy::scene::CAnimatedMeshSceneNode(
        0, 0, 0, -1, Vec(0, 0, 0), Vec(0, 0, 0), Vec(1, 1, 1));
    CHECK(mesh->getMaterialCount() == 0);
    CHECK(&mesh->getMaterial(-1) == fallback && &mesh->getMaterial(0) == fallback);
    CHECK(mesh->drop());

    // The recovered dummy node's matrix overrides its ordinary position/scale/rotation.
    daisy::scene::CDummyTransformationSceneNode* dummy =
        new daisy::scene::CDummyTransformationSceneNode(0, 0, -1);
    dummy->getRelativeTransformationMatrix().setTranslation(Vec(8, 9, 10));
    dummy->setPosition(Vec(100, 100, 100));
    dummy->OnPreRender();
    CHECK(dummy->getAbsolutePosition() == Vec(8, 9, 10));
    CHECK(dummy->drop());

    daisy::scene::CTriangleBBSelector* box = new daisy::scene::CTriangleBBSelector(root);
    ox::core::CTriangle3d<float> triangles[12];
    int count = -1;
    box->getTriangles(triangles, 12, count, 0);
    CHECK(count == 12);
    for (int i = 0; i < 12; ++i)
    {
        Vec points[] = {triangles[i].pointA, triangles[i].pointB, triangles[i].pointC};
        for (int j = 0; j < 3; ++j)
            CHECK((close(points[j].X, 9) || close(points[j].X, 11))
                && close(std::fabs(points[j].Y), 1) && close(std::fabs(points[j].Z), 1));
    }
    count = -1;
    box->getTriangles(triangles, 3, count, 0);
    CHECK(count == 3);
    CHECK(box->drop());
    CHECK(!child->drop());
    CHECK(root->drop() && other->drop());
    CHECK(nodesDestroyed == 3 && selectorsDestroyed == 2);
    std::puts("Scene node smoke passed: material fallback, null child, selector ownership/clear/alias, hierarchy, animation, dummy transform and box triangles");
    return 0;
}
