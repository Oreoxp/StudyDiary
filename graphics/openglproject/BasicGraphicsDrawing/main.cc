#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void DrawEvent();
void DrawClear();
void DrawLine();

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

const char* vertexShaderSoucre =
    "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
    "}\0";
const char* fragmentShaderSource =
    "#version 330 core\n"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
    "   FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
    "}\n\0";


void PaintLine(float* points, float* point_a, float* point_b, int count, int sub) {
  float x0 = point_a[0];
  float y0 = point_a[1];

  float x1 = point_b[0];
  float y1 = point_b[1];

  float k = (y1 - y0) / (x1 - x0);

  float d = 2 * (y0 - y1) * (x0 + 0.001f) + (x1 - x0) * (2 * y0 + 0.001f) +
          2 * x0 * y1 - 2 * x1 * y0;

  float x = x0, y = y0;
  int i = sub;
  if ( k < 1 && k >= 0) {
    while (x <= x1 && y <= y1) {
      points[i++] = x;
      points[i++] = y;
      points[i++] = 0;
      if (d < 0) {
        y += 0.001f;
        d += 2 * (x1 - x0) + 2 * (y0 - y1);
      } else {
        d += 2 * (y0 - y1);
      }
      x += 0.001f;
    }
  } else if (k >= 1) {
    while (x <= x1 && y <= y1) {
      points[i++] = x;
      points[i++] = y;
      points[i++] = 0;
      if (d < 0) {
        y += 0.001f * k;
        d += 2 * (x1 - x0) + 2 * (y0 - y1);
      } else {
        d += 2 * (y0 - y1);
      }
      x += 0.001f;
    }
  } else if (k > -1 && k <= 0) {
    while (x <= x1 && y >= y1) {
      points[i++] = x;
      points[i++] = y;
      points[i++] = 0;
      if (d > 0) {
        y -= 0.001f;
        d -= 2 * (x1 - x0) - 2 * (y0 - y1);
      } else if (d < 0) {
        d += 2 * (y0 - y1);
      }
      x += 0.001f;
    }
  } else if (k <= -1) {
    while (x <= x1 && y >= y1) {
      points[i++] = x;
      points[i++] = y;
      points[i++] = 0;
      if (d > 0) {
        y -= 0.001f * (-k);
        d -= 2 * (x1 - x0) - 2 * (y0 - y1);
      } else if (d < 0) {
        d += 2 * (y0 - y1);
      }
      x += 0.001f;
    }
  }
}

int main() {
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
  //接着创建窗口
  GLFWwindow* window =
      glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpengl", NULL, NULL);
  if (window == NULL) {
    std::cout << "窗口创建失败" << std::endl;
    glfwTerminate();  //终止
    return -1;
  }
  glfwMakeContextCurrent(window);
  glfwSetFramebufferSizeCallback(window,
                                 framebuffer_size_callback);  //监听窗口改变

  // glad 的问题
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return -1;
  }

  //下面是绘制三角形的主要代码
  int vertexShader = glCreateShader(GL_VERTEX_SHADER);  //创建顶点着色器
  glShaderSource(vertexShader, 1, &vertexShaderSoucre, NULL);
  glCompileShader(vertexShader);
  int success;
  char infoLog[512];
  glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);  //获取出错log
    std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
              << infoLog << std::endl;
  }

  int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);  //片段着色器
  glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
  glCompileShader(fragmentShader);
  glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);  //获取出错log
    std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n"
              << infoLog << std::endl;
  }
  //把两个着色器关联上一个着色器program
  int shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram,
                 vertexShader);  //顶点着色器和片段着色器都关联上着色器程序
  glAttachShader(shaderProgram, fragmentShader);
  glLinkProgram(shaderProgram);
  glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);  //出错log
    std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
              << infoLog << std::endl;
  }
  glDeleteShader(vertexShader);  //删除，顶点着色器和片段着色器都不需要再使用，数据都在着色器程序内
  glDeleteShader(fragmentShader);

  //三角形的三个顶点坐标
  /* float vertices[] = {
   -0.5f, -0.5f, 0.0f,
    0.5f, -0.5f, 0.0f, 
    0.0f, 0.5f, 0.0f, 
    0.5f,  0.0f, 0.0f,
    0.0f,  0.0f, 0.0f,
  };*/
  float vertices[120000];

  float point_a1[3] = {-1.0f, -1.0f, 0.0f};
  float point_b1[3] = {0.0f, 1.0f, 0.0f};
  PaintLine(vertices, point_a1, point_b1, 30000, 0);
  float point_a2[3] = {-1.0f, -1.0f, 0.0f};
  float point_b2[3] = {1.0f, 0.0f, 0.0f};
  PaintLine(vertices, point_a2, point_b2, 30000, 30000);
  float point_a3[3] = {-1.0f, 1.0f, 0.0f};
  float point_b3[3] = {1.0f, 0.0f, 0.0f};
  PaintLine(vertices, point_a3, point_b3, 30000, 60000);
  float point_a4[3] = {-1.0f, 1.0f, 0.0f};
  float point_b4[3] = {0.0f, -1.0f, 0.0f};
  PaintLine(vertices, point_a4, point_b4, 30000, 90000);


  //顶点数组对象：Vertex Array Object，VAO
  //顶点缓冲对象：Vertex Buffer Object，VBO
  unsigned int VBO, VAO;
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);
  glBindVertexArray(VAO);  //
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, 120000, vertices, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);



  //循环渲染
  while (!glfwWindowShouldClose(window)) {
    processInput(window);

    glClearColor(0.3f, 0.5f, 0.8f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(shaderProgram);  //使用这个着色器程序
    glBindVertexArray(VAO);
    //glDrawArrays(GL_TRIANGLES, 0, 3);  //绘制三角形
    glDrawArrays(GL_POINTS, 0, 120000);  //绘制点

    glfwSwapBuffers(window);
    glfwPollEvents();
  }
  //关闭的时候要删掉
  glDeleteVertexArrays(1, &VAO);
  glDeleteBuffers(1, &VBO);
  glfwTerminate();  //终止
  return 0;
}

// 处理所有输入：查询 GLFW 在这一帧是否按下/释放相关键并做出相应反应
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);
}

// glfw: 每当窗口大小改变（由操作系统或用户调整大小）时，此回调函数都会执行
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
  // 确保视口与新窗口尺寸匹配； 请注意，宽度和高度将明显大于视网膜显示器上指定的值。
  glViewport(0, 0, width, height);
}

// 绘制函数
void DrawEvent() {
  DrawClear();
  DrawLine();
}

void DrawClear() {
  glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
}

void DrawLine() {
  glClearColor(0.0, 0.0, 0.0, 0);
  glClear(GL_COLOR_BUFFER_BIT);
  /*
  glBegin(GL_TRIANGLES);  //开始绘制三角形
  glVertex2f(-0.5f,
             -0.5f);  //顶点坐标 ,这里一定要填三个坐标，否则绘制不出来三角形
  glVertex2f(0.0f, 0.5f);   //顶点坐标
  glVertex2f(0.5f, -0.5f);  //顶点坐标
  glEnd();                  //绘制结束
  glFlush();*/
  glRectf(-0.5f, -0.5f, 0.5f, 0.5f);
  glFlush();
}