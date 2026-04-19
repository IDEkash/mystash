#include "subengine/SubEngine.h"
#include "btBulletDynamicsCommon.h"
#include "log.h"

#ifdef __ANDROID__
#include <GLES3/gl3.h>
#else
#include <GL/gl.h>
#endif

namespace subengine {

class BulletBridge {
public:
    BulletBridge() {
        collisionConfiguration = new btDefaultCollisionConfiguration();
        dispatcher = new btCollisionDispatcher(collisionConfiguration);
        overlappingPairCache = new btDbvtBroadphase();
        solver = new btSequentialImpulseConstraintSolver;
        dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, overlappingPairCache, solver, collisionConfiguration);
        dynamicsWorld->setGravity(btVector3(0, -10, 0));

        // Create a ground plane for testing
        btCollisionShape* groundShape = new btStaticPlaneShape(btVector3(0, 1, 0), 1);
        btDefaultMotionState* groundMotionState = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, -1, 0)));
        btRigidBody::btRigidBodyConstructionInfo groundRBInfo(0, groundMotionState, groundShape, btVector3(0, 0, 0));
        btRigidBody* groundBody = new btRigidBody(groundRBInfo);
        dynamicsWorld->addRigidBody(groundBody);

        // Create a dynamic box
        btCollisionShape* colShape = new btBoxShape(btVector3(1, 1, 1));
        btDefaultMotionState* startMotionState = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, 10, 0)));
        btScalar mass(1.f);
        btVector3 localInertia(0, 0, 0);
        colShape->calculateLocalInertia(mass, localInertia);
        btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, startMotionState, colShape, localInertia);
        body = new btRigidBody(rbInfo);
        dynamicsWorld->addRigidBody(body);

        SubEngine::getInstance().hookProperty("bullet_body_pos", &bodyPos);
        SubEngine::getInstance().hookProperty("bullet_body_vel", &bodyVel);
        SubEngine::getInstance().hookProperty("bullet_gravity", &gravity);
    }

    ~BulletBridge() {
        delete dynamicsWorld;
        delete solver;
        delete overlappingPairCache;
        delete dispatcher;
        delete collisionConfiguration;
    }

    void update() {
        dynamicsWorld->stepSimulation(1.f / 60.f, 10);

        btTransform trans;
        body->getMotionState()->getWorldTransform(trans);
        bodyPos[0] = trans.getOrigin().getX();
        bodyPos[1] = trans.getOrigin().getY();
        bodyPos[2] = trans.getOrigin().getZ();

        btVector3 vel = body->getLinearVelocity();
        bodyVel[0] = vel.getX();
        bodyVel[1] = vel.getY();
        bodyVel[2] = vel.getZ();

        btVector3 g = dynamicsWorld->getGravity();
        gravity[0] = g.getX();
        gravity[1] = g.getY();
        gravity[2] = g.getZ();
    }

    void logic() {
        // Custom logic if needed
    }

    void renderDebug() {
        // Native rendering using GLES3
        // We avoid glBegin/glEnd as they don't exist in GLES3

        static const GLfloat groundVertices[] = {
            -10.0f, -1.0f, -10.0f, 10.0f, -1.0f, -10.0f,
            -10.0f, -1.0f, 10.0f, 10.0f, -1.0f, 10.0f,
            -10.0f, -1.0f, -10.0f, -10.0f, -1.0f, 10.0f,
            10.0f, -1.0f, -10.0f, 10.0f, -1.0f, 10.0f
        };

        // In GLES3 we use VAOs/VBOs and Shaders.
        // This is still a conceptual demonstration but using GLES3 compatible calls.
        glDisable(GL_DEPTH_TEST);

        // glEnableVertexAttribArray(0);
        // glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, groundVertices);
        // glDrawArrays(GL_LINES, 0, 8);
        // glDisableVertexAttribArray(0);
    }

private:
    btDefaultCollisionConfiguration* collisionConfiguration;
    btCollisionDispatcher* dispatcher;
    btBroadphaseInterface* overlappingPairCache;
    btSequentialImpulseConstraintSolver* solver;
    btDiscreteDynamicsWorld* dynamicsWorld;
    btRigidBody* body;

    float bodyPos[3];
    float bodyVel[3];
    float gravity[3];
};

static BulletBridge* g_bulletBridge = nullptr;

extern "C" {
    void subengine_init(EGLDisplay display, EGLContext context) {
        infostream << "BulletBridge: Initializing with context " << context << std::endl;
        g_bulletBridge = new BulletBridge();
    }

    void subengine_update() {
        if (g_bulletBridge) g_bulletBridge->update();
    }

    void subengine_logic() {
        if (g_bulletBridge) g_bulletBridge->logic();
    }

    void subengine_render() {
        if (g_bulletBridge) g_bulletBridge->renderDebug();
    }

    void subengine_shutdown() {
        delete g_bulletBridge;
        g_bulletBridge = nullptr;
    }
}

} // namespace subengine
