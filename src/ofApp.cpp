#include "ofMain.h"
#include "PointLight.h"
#include "ofxImGui.h"

#define NUM 800
#define LightNUM 10

class ofApp : public ofBaseApp{
    
    const int width = ofGetWidth();
    const int height = ofGetHeight();
    
    ofxImGui gui;
    float constant, linear, quadratic;
    
    ofBoxPrimitive box;
    ofConePrimitive cone;
    
    
    ofVec3f pos[NUM];
    PointLight pointlight[LightNUM];
    ofEasyCam cam;
    ofShader GeometryPass, LightingPass;
    
    GLuint gBuffer;
    GLuint gPosition, gNormal, gAlbedo;
    
    ofTexture texture, normal;
    
    ofMesh quad;
    int DebugMode;
    //--------------------------------------------------------------
    void setup()
    {
        float R = 400;
        for(int i = 0; i < LightNUM; i++)
        {
            pointlight[i].setPosition(ofVec3f(ofRandom(-R, R), ofRandom(-R, R), ofRandom(-R, R)));
            pointlight[i].setAmbient(0.0f, 0.0f, 0.0f);
            pointlight[i].setDiffuse(ofRandom(0.2), ofRandom(1.0), ofRandom(1.0));
            pointlight[i].setSpecular(1.0f, 1.0f, 1.0f);
        }
        
        R = 800;
        for(int i = 0; i < NUM; i++)
        {
            pos[i] = ofVec3f(ofRandom(-R, R), ofRandom(-R, R), ofRandom(-R, R));
        }
        
        
        
        gui.setup();
        cam.setNearClip(0.1);
        cam.setFarClip(2000.0);
        
        GeometryPass.load("shaders/gBuffer");
        LightingPass.load("shaders/lighting");
        
        ofDisableArbTex();
        ofLoadImage(texture, "tex/concrete.jpg");
        ofLoadImage(normal, "tex/random.png");
        
        ofEnableAntiAliasing();
        
        box.set(100);
        cone.set(20, 100);
        

        //gBuffer
        //-----------------------------------
        glGenFramebuffers(1, &gBuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
        
        glGenTextures(1, &gPosition);
        glBindTexture(GL_TEXTURE_2D, gPosition);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);
        
        glGenTextures(1, &gNormal);
        glBindTexture(GL_TEXTURE_2D, gNormal);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);
        
        glGenTextures(1, &gAlbedo);
        glBindTexture(GL_TEXTURE_2D, gAlbedo);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedo, 0);
        
        GLuint attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
        glDrawBuffers(3, attachments);
        
        GLuint rboDepth;
        glGenRenderbuffers(1, &rboDepth);
        glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
        
        // - Finally check if framebuffer is complete
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "Framebuffer not complete!" << std::endl;
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        
        
        quad.setMode(OF_PRIMITIVE_TRIANGLE_FAN);
        quad.addVertex(ofVec3f(1.0, 1.0, 0.0));
        quad.addTexCoord(ofVec2f(1.0f, 1.0f));
        quad.addVertex(ofVec3f(1.0, -1.0, 0.0));
        quad.addTexCoord(ofVec2f(1.0f, 0.0f));
        quad.addVertex(ofVec3f(-1.0, -1.0, 0.0));
        quad.addTexCoord(ofVec2f(0.0f, 0.0f));
        quad.addVertex(ofVec3f(-1.0, 1.0, 0.0));
        quad.addTexCoord(ofVec2f(0.0f, 1.0f));
        
        
        constant = 1.0f;
        linear = 0.0f;
        quadratic = 0.001f;
        DebugMode = 0;
        glEnable(GL_DEPTH_TEST);
    }
    
    //--------------------------------------------------------------
    void update()
    {
        
    }
    
    //--------------------------------------------------------------
    void draw()
    {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        gui.begin();
        ImGui::Text("Deferred Rendering");
        
        if(ImGui::Button("Final")) DebugMode = 0;
        else if(ImGui::Button("Position")) DebugMode = 1;
        else if(ImGui::Button("Normal")) DebugMode = 2;
        else if(ImGui::Button("Albedo")) DebugMode = 3;
        
        
        glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        cam.begin();
        ofMatrix4x4 viewMatrix = ofGetCurrentViewMatrix();
        cam.end();
        GeometryPass.begin();
        GeometryPass.setUniformMatrix4f("view", viewMatrix);
        GeometryPass.setUniformMatrix4f("projection", cam.getProjectionMatrix());
        GeometryPass.setUniformTexture("tex", texture, 0);
        GeometryPass.setUniformTexture("normal", normal, 1);
        for(int i = 0; i < NUM; i++)
        {
            pos[i].y -= 2.0;
            ofMatrix4x4 model;
            model.translate(pos[i]);
            GeometryPass.setUniformMatrix4f("model", model);
            
            cone.draw();
            if(pos[i].y < -900) pos[i].y = 900;
        }
        GeometryPass.end();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        
        
        LightingPass.begin();
        LightingPass.setUniformTexture("gPosition", GL_TEXTURE_2D, gPosition, 0);
        LightingPass.setUniformTexture("gNormal", GL_TEXTURE_2D, gNormal, 1);
        LightingPass.setUniformTexture("gAlbedo", GL_TEXTURE_2D, gAlbedo, 2);
        LightingPass.setUniform3f("ViewPos", cam.getPosition());
        
        for(int i = 0; i < LightNUM; i++)
        {
            ofVec3f lightPos =  pointlight[i].getPosition() * viewMatrix;
            LightingPass.setUniform3fv("light["+to_string(i)+"].position", &lightPos[0], 1);
            LightingPass.setUniform3fv("light["+to_string(i)+"].ambient", &pointlight[i].getAmbient()[0], 1);
            LightingPass.setUniform3fv("light["+to_string(i)+"].diffuse", &pointlight[i].getDiffuse()[0], 1);
            LightingPass.setUniform3fv("light["+to_string(i)+"].specular", &pointlight[i].getSpecular()[0], 1);
        }
        LightingPass.setUniform1f("constant", constant);
        LightingPass.setUniform1f("linear", linear);
        LightingPass.setUniform1f("quadratic", quadratic);
        LightingPass.setUniform1i("DebugMode", DebugMode);
        quad.draw(OF_MESH_FILL);
        LightingPass.end();

        
        gui.end();
    }
    
    
    //--------------------------------------------------------------
    void keyPressed(int key)
    {
        
    }
    
    //--------------------------------------------------------------
    void keyReleased(int key)
    {
        
    }
};

//========================================================================
int main()
{
    ofGLFWWindowSettings settings;
    settings.setGLVersion(3, 3);
    settings.width = 1280;
    settings.height = 720;
    ofCreateWindow(settings);
    
    // this kicks off the running of my app
    // can be OF_WINDOW or OF_FULLSCREEN
    // pass in width and height too:
    ofRunApp(new ofApp());
    
}
