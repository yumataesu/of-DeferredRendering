#include <random>

#include "ofMain.h"
#include "PointLight.h"
#include "ofxImGui.h"

#define NUM 400
#define LightNUM 3
#define KarnelSample 64

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
    ofShader GeometryPass, LightingPass, SSAOPass;
    
    GLuint gBuffer;
    GLuint gPosition, gNormal, gAlbedo, gDepth;
    GLuint  ssaoColorBuffer, ssaoColorBufferBlur;
    GLuint noiseTexture;
    GLuint ssaoFbo, ssaoBlurFBO;
    
    ofVec3f ssaoKernel[KarnelSample];
    
    ofTexture texture, normal;
    
    ofMesh quad;
    int DebugMode;
    //--------------------------------------------------------------
    void setup()
    {
        float R = 50;
        for(int i = 0; i < LightNUM; i++)
        {
            pointlight[i].setPosition(ofVec3f(ofRandom(-R, R), ofRandom(-R, R), ofRandom(-R, R)));
            pointlight[i].setAmbient(0.0f, 0.0f, 0.0f);
            pointlight[i].setDiffuse(ofRandom(0.2), ofRandom(1.0), ofRandom(1.0));
            pointlight[i].setSpecular(1.0f, 1.0f, 1.0f);
        }
        
        R = 30;
        for(int i = 0; i < NUM; i++)
        {
            pos[i] = ofVec3f(ofRandom(-R, R), ofRandom(-R, R), ofRandom(-R, R));
        }
        
        
        
        gui.setup();
        cam.setupPerspective(false, 80, 0.1f, 70.0f);
        cam.setDistance(70.0f);
        
        GeometryPass.load("shaders/gBuffer");
        LightingPass.load("shaders/lighting");
        SSAOPass.load("shaders/ssao");
        
        ofDisableArbTex();
        ofLoadImage(texture, "tex/concrete.jpg");
        ofLoadImage(normal, "tex/random.png");
        
        ofEnableAntiAliasing();
        
        box.set(15.0f);
        cone.set(20, 100);
        
        glEnable(GL_DEPTH_TEST);
        

        //gBuffer
        //-----------------------------------
        glGenFramebuffers(1, &gBuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
        
        glGenTextures(1, &gPosition);
        glBindTexture(GL_TEXTURE_2D, gPosition);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, NULL);
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
        
        glGenTextures(1, &gDepth);
        glBindTexture(GL_TEXTURE_2D, gDepth);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, width, height, 0, GL_RGB, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, gDepth, 0);
        
        GLuint attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
        glDrawBuffers(4, attachments);
        
        GLuint rboDepth;
        glGenRenderbuffers(1, &rboDepth);
        glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
        
        // - Finally check if framebuffer is complete
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "Framebuffer not complete!" << std::endl;
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        //----------------------------------------------------
        
        
        // Also create framebuffer to hold SSAO processing stage

        glGenFramebuffers(1, &ssaoFbo); glGenFramebuffers(1, &ssaoBlurFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, ssaoFbo);
        
        // - SSAO color buffer
        glGenTextures(1, &ssaoColorBuffer);
        glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RGB, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBuffer, 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "SSAO Framebuffer not complete!" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        
        
        glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
        glGenTextures(1, &ssaoColorBufferBlur);
        glBindTexture(GL_TEXTURE_2D, ssaoColorBufferBlur);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RGB, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBufferBlur, 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "SSAO Blur Framebuffer not complete!" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        
        
        // Sample kernel
        float randomFloats = ofRandom(1.0);
        for (int i = 0; i < KarnelSample; i++)
        {
            ofVec3f sample = ofVec3f(ofRandom(1.0) * 2.0 - 1.0, ofRandom(1.0) * 2.0 - 1.0, ofRandom(1.0));
            sample = sample.normalize();
            sample *= randomFloats;
            float scale = float(i) / 64.0;
            
            // Scale samples s.t. they're more aligned to center of kernel
            scale = lerp(0.1f, 1.0f, scale * scale);
            sample *= scale;
            ssaoKernel[i] = sample;
        }
        
        // Noise texture
        vector<ofVec3f> ssaoNoise;
        for (int i = 0; i < 16; i++)
        {
            ofVec3f noise = ofVec3f(ofRandom(1.0) * 2.0 - 1.0, ofRandom(1.0) * 2.0 - 1.0, 0.0f); // rotate around z-axis (in tangent space)
            ssaoNoise.push_back(noise);
        }
        
        glGenTextures(1, &noiseTexture);
        glBindTexture(GL_TEXTURE_2D, noiseTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        
        
        quad.setMode(OF_PRIMITIVE_TRIANGLE_FAN);
        quad.addVertex(ofVec3f(1.0, 1.0, 0.0));
        quad.addTexCoord(ofVec2f(1.0f, 1.0f));
        quad.addVertex(ofVec3f(1.0, -1.0, 0.0));
        quad.addTexCoord(ofVec2f(1.0f, 0.0f));
        quad.addVertex(ofVec3f(-1.0, -1.0, 0.0));
        quad.addTexCoord(ofVec2f(0.0f, 0.0f));
        quad.addVertex(ofVec3f(-1.0, 1.0, 0.0));
        quad.addTexCoord(ofVec2f(0.0f, 1.0f));
        
        constant = 0.02f;
        linear = 0.0f;
        quadratic = 0.00001f;
        DebugMode = 0;
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
        else if(ImGui::Button("Depth")) DebugMode = 4;
        
        
        //1.Geometry Psss
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
            pos[i].y -= 0.2;
            ofMatrix4x4 model;
            model.translate(pos[i]);
            GeometryPass.setUniformMatrix4f("model", model);
            
            box.draw();
            if(pos[i].y < -35) pos[i].y = 35;
        }
        
        GeometryPass.end();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        
        //2.SSAO Pass
        glBindFramebuffer(GL_FRAMEBUFFER, ssaoFbo);
        glClear(GL_COLOR_BUFFER_BIT);
        
        SSAOPass.begin();
        for(int i = 0; i < KarnelSample; i++)
        {
            SSAOPass.setUniform3fv("samples["+to_string(i)+"].offset", &ssaoKernel[i][0], 1);
        }
        
        SSAOPass.setUniformTexture("gPosition", GL_TEXTURE_2D, gPosition, 0);
        SSAOPass.setUniformTexture("gNormal", GL_TEXTURE_2D, gNormal, 1);
        SSAOPass.setUniformTexture("gDepth", GL_TEXTURE_2D, gDepth, 2);
        SSAOPass.setUniformTexture("noiseTexture", GL_TEXTURE_2D, noiseTexture, 3);
        SSAOPass.setUniformMatrix4f("projection", cam.getProjectionMatrix());
        
        quad.draw(OF_MESH_FILL);

        SSAOPass.end();
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

                
        LightingPass.begin();
        LightingPass.setUniformTexture("gPosition", GL_TEXTURE_2D, gPosition, 0);
        LightingPass.setUniformTexture("gNormal", GL_TEXTURE_2D, gNormal, 1);
        LightingPass.setUniformTexture("gAlbedo", GL_TEXTURE_2D, gAlbedo, 2);
        LightingPass.setUniformTexture("ssao", GL_TEXTURE_2D, ssaoColorBuffer, 3);
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
    
    float lerp(float a, float b, float f)
    {
        return a + f * (b - a);
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
    settings.resizable = false;
    ofCreateWindow(settings);
    
    // this kicks off the running of my app
    // can be OF_WINDOW or OF_FULLSCREEN
    // pass in width and height too:
    ofRunApp(new ofApp());
    
}
