// Copyright (C) 2016 West Virginia University.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//
//     * Neither the name of West Virginia University nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Please contact the author of this library if you have any questions.
// Author: Victor Fragoso (victor.fragoso@mail.wvu.edu)
// Author: Dustin Teel (dlteel@mix.wvu.edu)
// Author: Brandon Horn (<Add Email Here>)

// Use the right namespace for google flags (gflags).
#ifdef GFLAGS_NAMESPACE_GOOGLE
#define GLUTILS_GFLAGS_NAMESPACE google
#else
#define GLUTILS_GFLAGS_NAMESPACE gflags
#endif

#include <iostream>
#include <string>

// Include CImg library to load textures.
// The macro below disables the capabilities of displaying images in CImg.
#define cimg_display 0
#include <CImg.h>

// The macro below tells the linker to use the GLEW library in a static way.
// This is mainly for compatibility with Windows.
// Glew is a library that "scans" and knows what "extensions" (i.e.,
// non-standard algorithms) are available in the OpenGL implementation in the
// system. This library is crucial in determining if some features that our
// OpenGL implementation uses are not available.
#define GLEW_STATIC
#include <GL/glew.h>
// The header of GLFW. This library is a C-based and light-weight library for
// creating windows for OpenGL rendering.
// See http://www.glfw.org/ for more information.
#include <GLFW/glfw3.h>

//Include gflags and glog
#include <gflags/gflags.h>
#include <glog/logging.h>

// Shader program.
#include "shader_program.h"

// Model.
#include "model.h"
#include "model_loader.h"

// Transformation utils.
#include "transformations.h"

// Camera utils.
#include "camera_utils.h"
#include <iostream>

#define _USE_MATH_DEFINES
#include <gflags/gflags.h>

// Google flags.
// (<name of the flag>, <default value>, <Brief description of flat>)
// These will define global variables w/ the following format
// FLAGS_vertex_shader_filepath and
// FLAGS_fragment_shader_filepath.
// DEFINE_<type>(name of flag, default value, brief description.)
// types: string, int32, bool.
DEFINE_string(texture1_filepath, "",
              "Filepath of the texture 1.");
DEFINE_string(texture2_filepath, "",
              "Filepath of the texture 2.");
DEFINE_string(model_filepath, "", "Filepath of the first model.");
DEFINE_string(texture3_filepath, "", "Filepath of the texture 3");

// Annonymous namespace for constants and helper functions.
namespace {
    using wvu::Model;
    
    // Window dimensions.
    constexpr int kWindowWidth = 1280/*640*/;
    constexpr int kWindowHeight = 800/*480*/;
    
    // GLSL shaders.
    // Every shader should declare its version.
    // Vertex shader follows standard 3.3.0.
    // This shader declares/expexts an input variable named position. This input
    // should have been loaded into GPU memory for its processing. The shader
    // essentially sets the gl_Position -- an already defined variable -- that
    // determines the final position for a vertex.
    // Note that the position variable is of type vec3, which is a 3D dimensional
    // vector. The layout keyword determines the way the VAO buffer is arranged in
    // memory. This way the shader can read the vertices correctly.
    const std::string vertex_shader_src =
    "#version 330 core\n"
    "layout (location = 0) in vec3 position;\n"
    "layout (location = 1) in vec2 passed_texel;\n"
    "uniform mat4 model;\n"
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "out vec2 texel;\n"
    "\n"
    "void main() {\n"
    "gl_Position = projection * view * model * vec4(position, 1.0f);\n"
    "texel = passed_texel;\n"
    "}\n";
    
    // Fragment shader follows standard 3.3.0. The goal of the fragment shader is to
    // calculate the color of the pixel corresponding to a vertex. This is why we
    // declare a variable named color of type vec4 (4D vector) as its output. This
    // shader sets the output color to a (1.0, 0.5, 0.2, 1.0) using an RGBA format.
    const std::string fragment_shader_src =
    "#version 330 core\n"
    "in vec2 texel;\n"
    "out vec4 color;\n"
    "uniform sampler2D texture_sampler;\n"
    "void main() {\n"
    "color = texture(texture_sampler, texel);\n"
    "}\n";
    
    // -------------------- Texture helper functions -------------------------------
    GLuint LoadTexture(const std::string& texture_filepath) {
        cimg_library::CImg<unsigned char> image;
        image.load(texture_filepath.c_str());
        const int width = image.width();
        const int height = image.height();
        // OpenGL expects to have the pixel values interleaved (e.g., RGBD, ...). CImg
        // flatens out the planes. To have them interleaved, CImg has to re-arrange
        // the values.
        // Also, OpenGL has the y-axis of the texture flipped.
        image.permute_axes("cxyz");
        GLuint texture_id;
        glGenTextures(1, &texture_id);
        glBindTexture(GL_TEXTURE_2D, texture_id);
        // We are configuring texture wrapper, each per dimension,s:x, t:y.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // Define the interpolation behavior for this texture.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        /// Sending the texture information to the GPU.
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height,
                     0, GL_RGB, GL_UNSIGNED_BYTE, image.data());
        // Generate a mipmap.
        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);
        return texture_id;
    }
    
    // Error callback function. This function follows the required signature of
    // GLFW. See http://www.glfw.org/docs/3.0/group__error.html for more
    // information.
    static void ErrorCallback(int error, const char* description) {
        std::cerr << "ERROR: " << description << std::endl;
    }
    
    // Key callback. This function follows the required signature of GLFW. See
    // http://www.glfw.org/docs/latest/input_guide.html fore more information.
    static void KeyCallback(GLFWwindow* window,
                            int key,
                            int scancode,
                            int action,
                            int mods) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GL_TRUE);
        }
    }
    
    // Configures glfw.
    void SetWindowHints() {
        // Sets properties of windows and have to be set before creation.
        // GLFW_CONTEXT_VERSION_{MAJOR|MINOR} sets the minimum OpenGL API version
        // that this program will use.
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
        // Sets the OpenGL profile.
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        // Sets the property of resizability of a window.
        glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    }
    
    // Configures the view port.
    // Note: All the OpenGL functions begin with gl, and all the GLFW functions
    // begin with glfw. This is because they are C-functions -- C does not have
    // namespaces.
    void ConfigureViewPort(GLFWwindow* window) {
        int width;
        int height;
        // We get the frame buffer dimensions and store them in width and height.
        glfwGetFramebufferSize(window, &width, &height);
        // Tells OpenGL the dimensions of the window and we specify the coordinates
        // of the lower left corner.
        glViewport(0, 0, width, height);
    }
    
    // Clears the frame buffer.
    void ClearTheFrameBuffer() {
        // Sets the initial color of the framebuffer in the RGBA, R = Red, G = Green,
        // B = Blue, and A = alpha.
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        // Tells OpenGL to clear the Color buffer.
        glEnable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
    
    bool CreateShaderProgram(wvu::ShaderProgram* shader_program) {
        if (shader_program == nullptr) return false;
        shader_program->LoadVertexShaderFromString(vertex_shader_src);
        shader_program->LoadFragmentShaderFromString(fragment_shader_src);
        std::string error_info_log;
        if (!shader_program->Create(&error_info_log)) {
            std::cout << "ERROR: " << error_info_log << "\n";
        }
        if (!shader_program->shader_program_id()) {
            std::cerr << "ERROR: Could not create a shader program.\n";
            return false;
        }
        return true;
    }
    
    // Renders the scene.
    void RenderScene(const wvu::ShaderProgram& shader_program,
                     const Eigen::Matrix4f& projection,
                     const Eigen::Matrix4f& view,
                     std::vector<Model*>* models_to_draw,
                     GLFWwindow* window) {
        if(models_to_draw == nullptr || window == nullptr){
            std::cout << "Null pointer passed.  Could not render scene.";
            return;
        }
        for(int i = 0; i < models_to_draw->size(); i++){
            if(models_to_draw->at(i) == nullptr){
                std::cout << "Null pointer passed.  Could not render scene.";
                return;
            }
        }
        // Clear the buffer.
        ClearTheFrameBuffer();
        // Let OpenGL know that we want to use our shader program.
        shader_program.Use();
        // Render the models in a wireframe mode.
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        // Draw the models.
        // TODO: For every model in models_to_draw, call its Draw() method, passing
        // the view and projection matrices.
        //Rotate models within loops
        for(int i = 0; i < models_to_draw->size(); i++){
            models_to_draw->at(i)->Draw(shader_program, projection, view);
            //Now, rotate the Models
            //First, we get the current orientation
            Eigen::Vector3f current_orientation = models_to_draw->at(i)->orientation();
            //Now, change the current angle according to time
            const GLfloat rotation_speed = 50.0f;
            GLfloat current_angle = wvu::ConvertDegreesToRadians(rotation_speed * static_cast<GLfloat>(glfwGetTime()));
            //Encode the angle back into the orientation
            //Current orientation normalized
            Eigen::Vector3f normalized_orientation = current_orientation.normalized();
            Eigen::Vector3f new_orientation = current_angle * normalized_orientation;
            models_to_draw->at(i)->set_orientation(new_orientation);
        }
        // Let OpenGL know that we are done with our vertex array object.
        glBindVertexArray(0);
    }
    
    void ConstructModels(std::vector<Model*>* models_to_draw) {
        if(models_to_draw == nullptr){
            std::cout << "Null pointer passed.  Could not construct models.";
            return;
        }
        for(int i = 0; i < models_to_draw->size(); i++){
            if(models_to_draw->at(i) == nullptr){
                std::cout << "Null pointer passed.  Could not construct models.";
                return;
            }
        }
        
        // Load model.
        std::vector<Eigen::Vector3f> model_vertices;
        std::vector<Eigen::Vector2f> model_texels;
        std::vector<Eigen::Vector3f> model_normals;
        std::vector<wvu::Face> model_faces;
        if (!wvu::LoadObjModel(FLAGS_model_filepath,
                               &model_vertices,
                               &model_texels,
                               &model_normals,
                               &model_faces)) {
            LOG(ERROR) << "Could not load model: " << FLAGS_model_filepath;
            return;
        }
        LOG(INFO) << "Model succesfully loaded! "
        << " Num. Vertices=" << model_vertices.size()
        << " Num. Triangles=" << model_faces.size();
        
        
        Eigen::MatrixXf vertices(5, model_vertices.size());
        for (int col = 0; col < model_vertices.size(); ++col) {
            vertices.block(0, col, 3, 1) = model_vertices[col];
            vertices.block(3, col, 2, 1) = model_texels[col];
        }
        std::vector<GLuint> indices;
        for (int face_id = 0; face_id < model_faces.size(); ++face_id) {
            const wvu::Face& face = model_faces[face_id];
            indices.push_back(face.vertex_indices[0]);
            indices.push_back(face.vertex_indices[1]);
            indices.push_back(face.vertex_indices[2]);
        }
        
        
        Model* model;
        model = new Model(Eigen::Vector3f(1.0f, 1.0f, -1.0f),  // Orientation of object.
                    Eigen::Vector3f(-2.0f, 0.0f, -7.5f),  // Position of object.
                    vertices,
                    indices);
        
        GLuint texture_id = LoadTexture(FLAGS_texture1_filepath);
        model->set_texture(texture_id);
        models_to_draw->push_back(model);
        
        

        // TODO: Prepare your models here.
        // 1. Construct models by setting their vertices and poses.
        // 2. Create your models in the heap and add the pointers to models_to_draw.
        // 3. For every added model in models to draw, set the GPU buffers by
        // calling the method model method SetVerticesIntoGPU().
        
        // Prepare and render the pyramid
        Eigen::MatrixXf vertices_pyramid(5, 5);
        
        vertices_pyramid.block(0, 0, 3, 1) = Eigen::Vector3f(-1.0f, -1.0f, 1.0f);
        vertices_pyramid.block(3, 0, 2, 1) = Eigen::Vector2f(0.0f, 0.0f);
        
        vertices_pyramid.block(0, 1, 3, 1) = Eigen::Vector3f(1.0f, -1.0f, 1.0f);
        vertices_pyramid.block(3, 1, 2, 1) = Eigen::Vector2f(1.0f, 0.0f);
        
        vertices_pyramid.block(0, 2, 3, 1) = Eigen::Vector3f(1.0f, -1.0f, -1.0f);
        vertices_pyramid.block(3, 2, 2, 1) = Eigen::Vector2f(0.0f, 0.0f);
        
        vertices_pyramid.block(0, 3, 3, 1) = Eigen::Vector3f(-1.0f, -1.0f, -1.0f);
        vertices_pyramid.block(3, 3, 2, 1) = Eigen::Vector2f(1.0f, 0.0f);
        
        vertices_pyramid.block(0, 4, 3, 1) = Eigen::Vector3f(0.0f, 1.0f, 0.0f);
        vertices_pyramid.block(3, 4, 2, 1) = Eigen::Vector2f(0.5f, 1.0f);
        
        std::vector<GLuint> indices_pyramid = {
            0, 1, 4, //First triangle. (Front)
            1, 2, 4, //Second triangle. (Right)
            2, 3, 4, //Third triangle. (Back)
            3, 0, 4, //Fourth trinagle. (Left)
            0, 1, 3, //Fifth triangle. (Half of the base)
            3, 1, 2  //Sixth triangle  (Other half of the base)
        };
        
        
        //Prepare and render the cube for 20pts extra credit
        Eigen::MatrixXf vertices_cube(5, 8);
        
        vertices_cube.block(0, 0, 3, 1) = Eigen::Vector3f(0.0f, 1.0f, 0.0f);
        vertices_cube.block(3, 0, 2, 1) = Eigen::Vector2f(0.0f, 0.0f);
        
        vertices_cube.block(0, 1, 3, 1) = Eigen::Vector3f(0.0f, 0.0f, 0.0f);
        vertices_cube.block(3, 1, 2, 1) = Eigen::Vector2f(0.0f, 1.0f);
        
        vertices_cube.block(0, 2, 3, 1) = Eigen::Vector3f(1.0f, 1.0f, 0.0f);
        vertices_cube.block(3, 2, 2, 1) = Eigen::Vector2f(1.0f, 0.0f);
        
        vertices_cube.block(0, 3, 3, 1) = Eigen::Vector3f(1.0f, 0.0f, 0.0f);
        vertices_cube.block(3, 3, 2, 1) = Eigen::Vector2f(1.0f, 1.0f);
        
        vertices_cube.block(0, 4, 3, 1) = Eigen::Vector3f(1.0f, 1.0f, -1.0f);
        vertices_cube.block(3, 4, 2, 1) = Eigen::Vector2f(0.0f, 0.0f);
        
        vertices_cube.block(0, 5, 3, 1) = Eigen::Vector3f(1.0f, 0.0f, -1.0f);
        vertices_cube.block(3, 5, 2, 1) = Eigen::Vector2f(0.0f, 1.0f);
        
        vertices_cube.block(0, 6, 3, 1) = Eigen::Vector3f(0.0f, 1.0f, -1.0f);
        vertices_cube.block(3, 6, 2, 1) = Eigen::Vector2f(1.0f, 0.0f);
        
        vertices_cube.block(0, 7, 3, 1) = Eigen::Vector3f(0.0f, 0.0f, -1.0f);
        vertices_cube.block(3, 7, 2, 1) = Eigen::Vector2f(1.0f, 1.0f);
        
        
        
        
        std::vector<GLuint> indices_cube = {
            0, 1, 3,  // First triangle.
            0, 3, 2,  // Second triangle.
            2, 3, 5,  // Third triangle.
            2, 5, 4,  // Fourth triangle.
            4, 5, 7,  // Fifth triangle.
            4, 7, 6,  // Sixth triangle.
            0, 1, 7,  // Seventh triangle.
            0, 7, 6,  // Eigth triangle.
            0, 4, 6,  // Ninth triangle.
            0, 2, 4,  // Tenth triangle.
            1, 5, 7,  // Eleventh triangle.
            1, 3, 5   // Twelvth triangle.
            
        };
        
        
        
        
        Model* cube;
        cube = new Model(Eigen::Vector3f(1.0f, 1.0f, -1.0f),  // Orientation of object.
                         Eigen::Vector3f(1.0f, 0.0f, -7.5f),  // Position of object.
                         vertices_cube,
                         indices_cube);
        
        GLuint texture_id2 = LoadTexture(FLAGS_texture2_filepath);
        cube->set_texture(texture_id2);
        models_to_draw->push_back(cube);
        
        Model* pyramid;
        pyramid = new Model(Eigen::Vector3f(1.0f, 1.0f, -1.0f),  // Orientation of object.
                            Eigen::Vector3f(-1.0f, 0.0f, -7.5f),  // Position of object.
                            vertices_pyramid,
                            indices_pyramid);
        
        GLuint texture_id3 = LoadTexture(FLAGS_texture3_filepath);
        pyramid->set_texture(texture_id3);
        models_to_draw->push_back(pyramid);
        
        for(int i = 0; i < models_to_draw->size(); i++){
            models_to_draw->at(i)->SetVerticesIntoGpu();
        }
    }
    
    void DeleteModels(std::vector<Model*>* models_to_draw) {
        if(models_to_draw == nullptr){
            std::cout << "Null pointer passed.  Could not delete models.";
            return;
        }
        for(int i = 0; i < models_to_draw->size(); i++){
            if(models_to_draw->at(i) == nullptr){
                std::cout << "Null pointer passed.  Could not delete models.";
                return;
            }
        }
        // TODO: Implement me!
        // Call delete on each models to draw.
        for(int i = 0; i < models_to_draw->size(); i++){
            delete models_to_draw->at(i);
        }
    }
    
}  // namespace

int main(int argc, char** argv) {
    //Initialize gflags
    GLUTILS_GFLAGS_NAMESPACE::ParseCommandLineFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);
    // Initialize the GLFW library.
    if (!glfwInit()) {
        return -1;
    }
    
    // Setting the error callback.
    glfwSetErrorCallback(ErrorCallback);
    
    // Setting Window hints.
    SetWindowHints();
    
    // Create a window and its OpenGL context.
    const std::string window_name = "Assignment 3";
    GLFWwindow* window = glfwCreateWindow(kWindowWidth,
                                          kWindowHeight,
                                          window_name.c_str(),
                                          nullptr,
                                          nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    
    // Make the window's context current.
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetKeyCallback(window, KeyCallback);
    
    // Initialize GLEW.
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Glew did not initialize properly!" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    // Configure View Port.
    ConfigureViewPort(window);
    
    // Compile shaders and create shader program.
    wvu::ShaderProgram shader_program;
    if (!CreateShaderProgram(&shader_program)) {
        return -1;
    }
    
    // Construct the models to draw in the scene.
    std::vector<Model*> models_to_draw;
    ConstructModels(&models_to_draw);
    
    // Construct the camera projection matrix.
    const float field_of_view = wvu::ConvertDegreesToRadians(45.0f);
    const float aspect_ratio = static_cast<float>(kWindowWidth / kWindowHeight);
    const float near_plane = 0.1f;
    const float far_plane = 10.0f;
    const Eigen::Matrix4f& projection =
    wvu::ComputePerspectiveProjectionMatrix(field_of_view, aspect_ratio,
                                            near_plane, far_plane);
    const Eigen::Matrix4f view = Eigen::Matrix4f::Identity();
    
    // Loop until the user closes the window.
    while (!glfwWindowShouldClose(window)) {
        // Render the scene!
        RenderScene(shader_program, projection, view, &models_to_draw, window);
        
        // Swap front and back buffers.
        glfwSwapBuffers(window);
        
        // Poll for and process events.
        glfwPollEvents();
    }
    
    // Cleaning up tasks.
    DeleteModels(&models_to_draw);
    // Destroy window.
    glfwDestroyWindow(window);
    // Tear down GLFW library.
    glfwTerminate();
    
    return 0;
}
