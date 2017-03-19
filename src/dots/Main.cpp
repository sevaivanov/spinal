/*
 * @file
 * @author Vsevolod (Seva) Ivanov
*/

#include <stdio.h>
#include <iostream>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Window.hpp"
#include "Shader.hpp"
#include "Camera.hpp"

Window* window;
Shader* shader;
Camera* camera;

GLuint vbo, vao, ebo;

glm::mat4 model;
glm::mat4 view;
glm::mat4 projection;

std::vector<glm::vec3> vertices;
std::vector<GLint> vertices_i;

std::string data_f = "data/five_y";
GLfloat rotate_angle = 1 / 20;
GLenum render_m = GL_POINTS;

void rotate(const glm::vec3 spin)
{
    model = glm::rotate(model, spin.x, glm::vec3(1, 0, 0));
    model = glm::rotate(model, spin.y, glm::vec3(0, 1, 0));
    model = glm::rotate(model, spin.z, glm::vec3(0, 0, 1));
}

void gen_vertices_i()
{
    vertices_i.clear();

    if (render_m == GL_POINTS)
    {
        for (uint8_t p = 0; p < vertices.size() - 1; p++)
        {
            vertices_i.push_back(p);
            //printf("Adding point p%i\n", p);
        }
    }
    else if (render_m == GL_LINES)
    {
        for (uint8_t p = 0; p < vertices.size() - 2; p++)
        {
            // edge
            vertices_i.push_back(p);
            vertices_i.push_back(p + 1);
            //printf("Adding edge between p%i <-> p%i\n", p, p + 1);
        }
    }
}

// callbacks {

static void framebuffer_size_cb(GLFWwindow* w, int width, int height)
{
    window->width(width);
    window->height(height);
    glViewport(0, 0, width, height);
}

static void key_cb(GLFWwindow* w, int key, int scancode, int action, int mode)
{
    //printf("keyboard: %i\n", key);

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(w, GL_TRUE);
    }

    switch (key)
    {
        case GLFW_KEY_LEFT:
            rotate(glm::vec3(0, rotate_angle, 0));
            break;

        case  GLFW_KEY_RIGHT:
            rotate(glm::vec3(0, -1 * rotate_angle, 0));
            break;

        case GLFW_KEY_UP:
            rotate(glm::vec3(rotate_angle, 0, 0));
            break;

        case GLFW_KEY_DOWN:
            rotate(glm::vec3(-1 * rotate_angle, 0, 0));
            break;

        case GLFW_KEY_W:
            camera->move_down();
            break;

        case GLFW_KEY_S:
            camera->move_up();
            break;

        case GLFW_KEY_A:
            camera->move_left();
            break;

        case GLFW_KEY_D:
            camera->move_right();
            break;

        case GLFW_KEY_P:
            render_m = GL_POINTS;
            gen_vertices_i();
            break;

        case GLFW_KEY_L:
            render_m = GL_LINES;
            gen_vertices_i();
            break;
    }
}

static void mouse_scroll_cb(GLFWwindow *w, double xoffset, double yoffset)
{
    if (yoffset > 0)        camera->move_forward();
    else if (yoffset < 0)   camera->move_backward();
}

// } callbacks

bool load_data_file()
{
    std::ifstream ifs;
    ifs.open(data_f);

	if (!ifs.is_open())
        return false;

    GLfloat x, y, z;
    unsigned short n;

    ifs >> n;
    printf("Found %i data points\n", n);

    vertices.clear();

    for(unsigned short i = 0; i < n; i++)
    {
        ifs >> x >> y >> z;
        vertices.push_back(glm::vec3(x, y, z));
        printf("(%f, %f, %f)\n", x, y, z);
    }

    ifs.close();
    return true;
}

void init_buffers()
{
    glGenBuffers(1, &vbo);
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &ebo);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(glm::vec3) * vertices.size(),
                 &vertices[0], GL_STATIC_DRAW);

    // has to be before ebo bind
    glBindVertexArray(vao);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 sizeof(vertices_i) * vertices_i.size(),
                 &vertices_i[0],
                 GL_STATIC_DRAW);

    // enable vao -> vbo pointing
    glEnableVertexAttribArray(0);
    // setup formats of my vao attributes
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), NULL);

    // unbind vbo
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    // unbind vao
    glBindVertexArray(0);
}

void render()
{
    shader->use();

    // locate in shaders gpu
    GLint modelLoc = glGetUniformLocation(shader->programId, "model");
    GLint viewLoc = glGetUniformLocation(shader->programId, "view");
    GLint projLoc = glGetUniformLocation(shader->programId, "projection");

    // send to shaders on gpu
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glBindVertexArray(vao);
        glDrawElements(render_m, vertices_i.size(), GL_UNSIGNED_SHORT, 0);
    glBindVertexArray(0);
}

void draw_loop()
{
    while (!glfwWindowShouldClose(window->get()))
    {
        glfwPollEvents();

        // clear the colorbuffer
        glClearColor(255, 255, 255, 0); // background color
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        view = glm::translate(camera->view(), glm::vec3(0.0f, 0.0f, -1.0f));

        projection = glm::perspective(
            45.0f,
            (GLfloat) window->width() / (GLfloat) window->height(),
            0.1f, 100.0f);

        render();

        glfwSwapBuffers(window->get());
    }
}

int main(int argc, char *argv[])
{
    // init

    camera = new Camera();
    window = new Window(800, 800, "Spinal");

    glfwSetKeyCallback(window->get(), key_cb);
    glfwSetFramebufferSizeCallback(window->get(), framebuffer_size_cb);
    glfwSetScrollCallback(window->get(), mouse_scroll_cb);

    glewExperimental = GL_TRUE;
    glewInit();

    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_DEPTH_TEST);

    glViewport(0, 0, window->width(), window->height());

    shader = new Shader("shaders/default.vs", "shaders/default.fs");

 	if (!load_data_file())
    {
        std::cout << "Can't open data file '" << data_f << "'" << std::endl;
        return 1;
    }
    gen_vertices_i();

    init_buffers();

    draw_loop();

    glDeleteBuffers(1, &ebo);
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);

    delete shader;
    delete camera;
    delete window;

    return 0;
}
