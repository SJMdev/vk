#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define FMT_HEADER_ONLY
#include <fmt/core.h> 
#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <array>

// window parameters
const int window_width = 3840;
const int window_height = 2160;
// const int window_width  = 1920;
// const int window_height = 1080;
const int gl_major_version = 4;
const int gl_minor_version = 5;

// camera (perspective)
const float g_fov = 90.0f;
const float aspect_ratio = static_cast<float>(window_width) / static_cast<float>(window_height);
const float z_near = 0.5f;
const float z_far = 10000.0f;
 
// camera (world position etc)
const float ground_height = 0.3f;
const float camera_x = -50.1f;
const float camera_y = 50.0f;
const float camera_z = 75.0f;
const float camera_speed = 15.0f; // why are these different?
const float camera_movement_speed = 4.0f;
const float camera_zoom_speed = 10.0f;

// simulation
const int particle_count = 10000000;
// const int particle_count = 1000;
const int attractor_count = 8;

std::array<glm::vec4, particle_count>  compute_positions;
std::array<float, particle_count>      compute_lifetimes;
std::array<glm::vec4, particle_count>  compute_velocities;
std::array<glm::vec4, attractor_count> compute_attractors;

// arbitrary constants
const float E = 2.71828183f;
const float pi = 3.14159265f;
const float epsilon = 0.0001f; // what kind of epsilon?

// rendering stuff (these names are HORRIBLE)
int rendered_frames = 0;
double frame_time = 0.0;
double counter = 0.0;
double fps = 0.0;
uint32_t VAO;

// compute shader
const int workgroup_size = 128;


#define INVALID_SHADER_PROGRAM_ID 0

#include <iostream>
#include <fstream>
#include <string>
std::string file_to_string(const std::string& file_name) {
    std::ifstream file(file_name);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << file_name << std::endl;
        return "";
    }

    std::string content;
    std::string line;
    while (std::getline(file, line)) {
        content += line + "\n";
    }

    file.close();
    return content;
}

static void check_for_errors(int shader_id)
{
    GLint max_length = 0;
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &max_length);
    std::vector<GLchar> info_log(max_length);        
    if (info_log.size() > 0)
    {
        glGetShaderInfoLog(shader_id, max_length, &max_length, &info_log[0]);
        std::string string_log = std::string(info_log.begin(), info_log.end());
        fmt::print("failed to compile shader in {}. GL errror: {}\n", __func__, string_log);
    }
    GLint shader_compiled = GL_FALSE; 
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &shader_compiled);

    //@FIXME(SMIA): uh... this should mos def not be deleting shaders.
    if (shader_compiled != GL_TRUE)
    {
        fmt::print("shader did not compile. bail!\n");
        glDeleteShader(shader_id); // Don't leak the shader.
        exit(1);
    }
}

static int create_point_shader_program(const char* vertex_path, const char* fragment_path)
{
    const uint32_t shader_program = glCreateProgram();

    int vertex_shader_id   = glCreateShader(GL_VERTEX_SHADER);
    int fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

    // compile vertex shader.
    std::string vertex_shader_src = file_to_string(vertex_path);
    const char* vertex_shader_c_str = vertex_shader_src.c_str();
    glShaderSource(vertex_shader_id, 1, &vertex_shader_c_str, NULL);
    glCompileShader(vertex_shader_id);
    check_for_errors(vertex_shader_id);
    glAttachShader(shader_program, vertex_shader_id);

    // compile fragment shader.
    std::string fragment_shader_src = file_to_string(fragment_path);
    const char* fragment_shader_c_str = fragment_shader_src.c_str();
    glShaderSource(fragment_shader_id, 1, &fragment_shader_c_str, NULL);
    glCompileShader(fragment_shader_id);
    check_for_errors(fragment_shader_id);
    glAttachShader(shader_program, fragment_shader_id);

    glLinkProgram(shader_program);

    // after shaders are linked, we are free to delete them.
    glDeleteShader(vertex_shader_id); 
    glDeleteShader(fragment_shader_id);

    return shader_program;
}

static int create_compute_shader_program(const char* compute_path)
{
    const uint32_t shader_program = glCreateProgram();

    int compute_shader_id = glCreateShader(GL_COMPUTE_SHADER);

    // compile compute shader
    std::string compute_shader_src = file_to_string(compute_path);
    const char* compute_shader_c_str = compute_shader_src.c_str();
    glShaderSource(compute_shader_id, 1, &compute_shader_c_str, NULL);
    glCompileShader(compute_shader_id);
    check_for_errors(compute_shader_id);
    glAttachShader(shader_program, compute_shader_id);

    glLinkProgram(shader_program);
    glDeleteShader(compute_shader_id);

    return shader_program;
}

// find some way to remove the attractor buffer, but better to pass it in now.
static void simulate(
    float dt, 
    uint32_t particle_shader_id,
    uint32_t compute_shader_id,
    uint32_t position_buffer,
    uint32_t velocity_buffer,
    uint32_t attractor_buffer,
    uint32_t lifetime_buffer)
{
    counter += dt;
    fmt::print("dt: {}\n",dt);
    
    if (counter >= 1.0)
        counter = 0.0 - epsilon;
    
    if (counter <= 0.0)
        fps = (int) 1.0 / dt;


    // fixme: this should definitely not be the way. just create a uniform buffer that we update?
    // update the attractors (to move the particles around/)    

    glBindBuffer(GL_ARRAY_BUFFER, attractor_buffer);
    // modify gpu-based buffer, whatever was in there can be discarded.
    glm::vec4* attractor_buffer_ptr = (glm::vec4*)glMapBufferRange(GL_ARRAY_BUFFER, 0, attractor_count * sizeof(glm::vec4), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
    for (size_t idx = 0; idx < attractor_count; ++idx)
    {
        attractor_buffer_ptr[idx].x = (rand() % 100) / 500.0 - (rand() % 100) / 500.0;
        attractor_buffer_ptr[idx].y = (rand() % 100) / 500.0 - (rand() % 100) / 500.0;
        attractor_buffer_ptr[idx].z = (rand() % 100) / 500.0 - (rand() % 100) / 500.0;

        attractor_buffer_ptr[idx].x *= sinf(counter);
        attractor_buffer_ptr[idx].y *= cosf(counter);
        attractor_buffer_ptr[idx].z = tanf(counter);

        // attractor_buffer_ptr[idx].x = sinf(counter) * (rand() % 500) / 10.0;
        // attractor_buffer_ptr[idx].y = cosf(counter) * (rand() % 500) / 10.0;
        // attractor_buffer_ptr[idx].z = tanf(counter);
    }
    glUnmapBuffer(GL_ARRAY_BUFFER);

    {
        glDisable(GL_CULL_FACE);
        glUseProgram(compute_shader_id);
        glUniform1fv(glGetUniformLocation(compute_shader_id, "dt"), 1, &dt);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, position_buffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, velocity_buffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, attractor_buffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, lifetime_buffer);

        glDispatchCompute(particle_count / workgroup_size, 1, 1);
        glMemoryBarrier(GL_ALL_BARRIER_BITS);
        glUseProgram(0);
    }

    //@TODO: does the simulation not actually update the position_buffer?
    // or the lifetime buffer?
    // check before and after!
    {
        glUseProgram(particle_shader_id);
        glEnable(GL_CULL_FACE);
        glBindBuffer (GL_ARRAY_BUFFER, position_buffer);
        // default VAO?
        glBindVertexArray(VAO);
        glPointSize(2.0);
        glDrawArrays(GL_POINTS, 0, particle_count);
    }

}


//@FIXME(SMIA):
// this is super stupid: we keep resetting the glfw time to 0.0 
// as a sort of clean "delta time" hack since last invocation.
static float get_dt()
{
    float dt = glfwGetTime();
    glfwSetTime(0.0);

    rendered_frames += 1;
    frame_time += dt;

    if (rendered_frames >= 1000) {
        printf ("fps: %i\n", (int) (1.0 / ((double)frame_time / (double)rendered_frames)));
        frame_time = 0.0;
        rendered_frames = 0;
    }
    return static_cast<float>(dt);
}

// to get openGL debug info, you need a glfw version > 3.3 (4.5 should work)
// as well as a OPENGL_DEBUG_CONTEXT!
int main() {

    // note to self: do not call gl functions before glad is live.
    GLFWwindow* window = nullptr;

    // glfw init
    {
        glfwInit();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, gl_major_version);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, gl_minor_version);
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        
        window = glfwCreateWindow(window_width, window_height, "Compute Shader particle effects", NULL, NULL);
        if (window == nullptr)
        {
            fmt::print("failed to create window\n");
            glfwTerminate();
            return -1;
        }

        glfwMakeContextCurrent(window);
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
            fmt::print("failed to initialize GLAD\n");
            return -1;
        }

        // enable vsync
        // glfwSwapInterval(1);
    }

    // gl init 
    {
        glClearColor(0.0, 0.0, 0.0, 0.0);
        glClearDepth(1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        glDisable(GL_CULL_FACE);

        // set the window region to the whole screen.
        glViewport(0, 0, window_width, window_height);

    }

    int compute_shader  = create_compute_shader_program("shaders/particle.comp");
    int particle_shader = create_point_shader_program("shaders/particle.vert", "shaders/particle.frag");

    // set up uniforms.
    {
        glm::mat4 perspective = glm::perspective(g_fov, aspect_ratio,z_near,z_far);
        glm::vec3 target_position{0.0, 0.0, 0.0};
        glm::vec3 camera_position{camera_x, camera_y, camera_z};
        glm::vec3 up_direction{0.0, 1.0, 0.0};
        glm::mat4 view = glm::lookAt(camera_position, target_position, up_direction);

        // bind the correct shader before setting uniforms.
        glUseProgram(particle_shader);
        int32_t projection_matrix_uniform_location = glGetUniformLocation(particle_shader, "projection_matrix");
        int32_t view_matrix_uniform_location       = glGetUniformLocation(particle_shader, "view_matrix");
        glUniformMatrix4fv(projection_matrix_uniform_location, 1, GL_FALSE, glm::value_ptr(perspective));
        glUniformMatrix4fv(view_matrix_uniform_location,      1, GL_FALSE, glm::value_ptr(view));
    }
    // at this point we should at least be good to go from the point_shader perspective.

    for (auto& attractor:  compute_attractors) {
        attractor.x = (rand() % 500) / 30.0 - (rand() % 500) / 30.0;
        attractor.y = (rand() % 500) / 30.0 - (rand() % 500) / 30.0;
        attractor.z = (rand() % 500) / 30.0 - (rand() % 500) / 30.0;
        attractor.w = 0;
    }

    for (auto& position: compute_positions)
    {
        position.x = (rand() % 100) / 500.0 - (rand() % 100) / 500.0;
        position.y = (rand() % 100) / 500.0 - (rand() % 100) / 500.0;
        position.z = (rand() % 100) / 500.0 - (rand() % 100) / 500.0;
    }

    for (auto& velocity: compute_velocities)
    {
        velocity.x = (rand() % 500) / 30.0 - (rand() % 500) / 30.0;
        velocity.y = (rand() % 500) / 30.0 - (rand() % 500) / 30.0;
        velocity.z = (rand() % 500) / 30.0 - (rand() % 500) / 30.0;
        velocity.w = 0;
    }

    uint32_t attractor_buffer{};
    glGenBuffers(1, &attractor_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, attractor_buffer);
    glBufferData(GL_ARRAY_BUFFER, attractor_count * sizeof(glm::vec4), compute_attractors.data(), GL_DYNAMIC_COPY);

    uint32_t position_buffer{};
    glGenBuffers    (1, &position_buffer);
    glBindBuffer    (GL_ARRAY_BUFFER, position_buffer);
    glBufferData    (GL_ARRAY_BUFFER, particle_count * sizeof(glm::vec3), compute_positions.data(), GL_DYNAMIC_COPY);

    uint32_t lifetime_buffer{};
    glGenBuffers    (1, &lifetime_buffer);
    glBindBuffer    (GL_ARRAY_BUFFER, lifetime_buffer);
    glBufferData    (GL_ARRAY_BUFFER, particle_count * sizeof(float), compute_lifetimes.data(), GL_DYNAMIC_COPY);

    uint32_t velocity_buffer{};
    glGenBuffers    (1, &velocity_buffer);
    glBindBuffer    (GL_ARRAY_BUFFER, velocity_buffer);
    glBufferData    (GL_ARRAY_BUFFER, particle_count * sizeof(glm::vec4), compute_velocities.data(), GL_DYNAMIC_COPY);

    // create VAO?
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    glBindBuffer (GL_ARRAY_BUFFER, position_buffer);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, lifetime_buffer);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(float), 0);


    double dt = 0.0;
    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT);
        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------

        // simulate?
        float new_dt = get_dt();
        simulate(
            dt, 
            particle_shader,
            compute_shader,
            position_buffer,
            velocity_buffer,
            attractor_buffer,
            lifetime_buffer);

        dt = new_dt;

        glfwSwapBuffers(window);

        glfwPollEvents();
    }


}