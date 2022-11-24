#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <windows.h>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void DrawEvent();
void DrawClear();
void DrawLine();

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

/*
    顶点着色器GLSL源码
    GLSL版本号和OpenGL的版本是匹配的，使用openGL3.3版本则使用GLSL的330
    使用in关键字，在顶点着色器中声明所有的输入顶点属性
    layout (location = 0)设定了输入变量(顶点属性)的位置值；并且后面链接顶点属性设置的时候会通过顶点属性位置值进行绑定
    vec3 表示三个分量的值(注意每个值都是浮点数类型)
    第二行就是表示在位置0的地方有一个三个分量的输入变量aPos
    将输入的三维aPos转换为四维并赋值给全局变量gl_Position
*/
const char* vertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"   //位置变量的属性位置值为 0
"layout (location = 1) in vec3 aColor;\n" //颜色变量的属性位置值为 1
"out vec4 vertexColor;\n"
"void main()\n"
"{\n"
"   vertexColor = vec4(aColor, 1.0);\n"
"   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
"}\0";
/*
    片段着色器GLSL源码
    GLSL版本号和OpenGL的版本是匹配的，使用openGL3.3版本则使用GLSL的330
    out 表示输出变量 四维向量的变量FragColor
    暂时自定义为 一个Alpha值为1.0(1.0代表完全不透明)的橘黄色的vec4赋值给颜色输出。
    RGBA的四分量向量
    片段着色器所做的是计算像素最后的颜色输出。为了让事情更简单暂时自定义为  我们的片段着色器将会一直输出橘黄色。
*/
const char* fragmentShaderSource = "#version 330 core\n"
"out vec4 FragColor;\n"
"in vec4 vertexColor;\n"
"void main()\n"
"{\n"
"   FragColor = vertexColor;\n"// 将ourColor设置为我们从顶点数据那里得到的输入颜色
"}\n\0";

#define my_min(a,b,c) min(min(a,b),c)
#define my_max(a,b,c) max(max(a,b),c)
#define BUFF_ARR_SIZE 120000000

#define MSAA_STEPINT 0.008f

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

void PaintTriangle(float* points, float* point_a, float* point_b, float* point_c, int count, int sub) {
    float x0 = point_a[0];
    float y0 = point_a[1];
    float x1 = point_b[0];
    float y1 = point_b[1];
    float x2 = point_c[0];
    float y2 = point_c[1];

    float xmin = my_min(x0, x1, x2);
    float xmax = my_max(x0, x1, x2);
    float ymin = my_min(y0, y1, y2);
    float ymax = my_max(y0, y1, y2);
    float x = xmin, y = ymin;
    float a, b, c;
    int i = sub;
    while (y < ymax) {
        x = xmin;
        while (x < xmax) {
            a = ((y1 - y2) * x + (x2 - x1) * y + x1 * y2 - x2 * y1) / ((y1 - y2) * x0 + (x2 - x1) * y0 + x1 * y2 - x2 * y1);
            b = ((y2 - y0) * x + (x0 - x2) * y + x2 * y0 - x0 * y2) / ((y2 - y0) * x1 + (x0 - x2) * y1 + x2 * y0 - x0 * y2);
            c = ((y0 - y1) * x + (x1 - x0) * y + x0 * y1 - x1 * y0) / ((y0 - y1) * x2 + (x1 - x0) * y2 + x0 * y1 - x1 * y0);
            if (a >= 0 && b >= 0 && c >= 0) {
              if ((a > 0 ||
                   ((y1 - y2) * x0 + (x2 - x1) * y0 + x1 * y2 - x2 * y1) *
                       ((y1 - y2) * (-1) + (x2 - x1) * (-1) + x1 * y2 -
                        x2 * y1)) &&
                  (b > 0 ||
                   ((y2 - y0) * x1 + (x0 - x2) * y1 + x2 * y0 - x0 * y2) *
                       ((y2 - y0) * (-1) + (x0 - x2) * (-1) + x2 * y0 -
                        x0 * y2)) &&
                  (c > 0 ||
                   ((y0 - y1) * x2 + (x1 - x0) * y2 + x0 * y1 - x1 * y0) *
                       ((y0 - y1) * (-1) + (x1 - x0) * (-1) + x0 * y1 -
                        x1 * y0))) {    //该点在图形内
                points[i++] = x;
                points[i++] = y;
                points[i++] = 0;

                if (i != sub + 3) {     //步进坐标
                  points[i++] = points[i - 6] + 0.000003f;
                  points[i++] = points[i - 6] + 0.000002f;
                  points[i++] = points[i - 6] + 0.000001f;
                } else {                //set color
                  points[i++] = 0 + 0.0001f;
                  points[i++] = 0 + 0.0002f;
                  points[i++] = 0 + 0.0003f;
                }
              }
            }
            x += 0.002f;
        }
        y += 0.002f;
    }
}

void PaintTriangleWithMSAA(float* points,
                           float* point_a,
                           float* point_b,
                           float* point_c,
                           int count,
                           int sub) {
  float x0 = point_a[0];
  float y0 = point_a[1];
  float x1 = point_b[0];
  float y1 = point_b[1];
  float x2 = point_c[0];
  float y2 = point_c[1];

  float xmin = my_min(x0, x1, x2);
  float xmax = my_max(x0, x1, x2);
  float ymin = my_min(y0, y1, y2);
  float ymax = my_max(y0, y1, y2);
  float x = xmin, y = ymin;
  float a, b, c;
  float bu;

  int i = sub;

  points[i++] = x;
  points[i++] = y;
  points[i++] = 0;

  points[i++] = 0.1f;
  points[i++] = 0.1f;
  points[i++] = 0.1f;
  int in_count;

  while (y < ymax) {
    x = xmin;
    while (x < xmax) {
      in_count = 0;
      for (int m = 0; m < 4; ++m) {
        for (int n = 0; n < 4; ++n) {
          a = ((y1 - y2) * x + (x2 - x1) * y + x1 * y2 - x2 * y1) /
              ((y1 - y2) * x0 + (x2 - x1) * y0 + x1 * y2 - x2 * y1);
          b = ((y2 - y0) * x + (x0 - x2) * y + x2 * y0 - x0 * y2) /
              ((y2 - y0) * x1 + (x0 - x2) * y1 + x2 * y0 - x0 * y2);
          c = ((y0 - y1) * x + (x1 - x0) * y + x0 * y1 - x1 * y0) /
              ((y0 - y1) * x2 + (x1 - x0) * y2 + x0 * y1 - x1 * y0);
          if (a >= 0 && b >= 0 && c >= 0) {
            if ((a > 0 ||
                 (((y1 - y2) * x0 + (x2 - x1) * y0 + x1 * y2 - x2 * y1) *
                  ((y1 - y2) * (-1) + (x2 - x1) * (-1) + x1 * y2 - x2 * y1)) >
                     0) &&
                (b > 0 ||
                 (((y2 - y0) * x1 + (x0 - x2) * y1 + x2 * y0 - x0 * y2) *
                  ((y2 - y0) * (-1) + (x0 - x2) * (-1) + x2 * y0 - x0 * y2)) >
                     0) &&
                (c > 0 ||
                 (((y0 - y1) * x2 + (x1 - x0) * y2 + x0 * y1 - x1 * y0) *
                  ((y0 - y1) * (-1) + (x1 - x0) * (-1) + x0 * y1 - x1 * y0))) >
                    0) {  //该点在图形内
              in_count++;
              /*std::cout << "point:[i] = (" << i << "," << i + 1 << "," << i +
                 2
                        << ")" << std::endl;*/
              points[i++] = x;
              points[i++] = y;
              points[i++] = 0;
              i += 3;
            }
          }
          x += MSAA_STEPINT / 4;
        }
        y += MSAA_STEPINT / 4;
        x -= MSAA_STEPINT;
      }
      y -= MSAA_STEPINT;
      x += MSAA_STEPINT;

      i -= 96;
      // auto ss = 1.0 - (in_count / 16.0) + 1.0;
      auto ss = in_count / 16.0;
      if (in_count != 0) {
        for (int m = 0; m < 4; ++m) {
          for (int n = 0; n < 4; ++n) {
            int ssss = i + m * 24 + (n * 6 + 3);
            /*std::cout << "color:[i] = (" << ssss << "," << ssss + 1 << ","
                      << ssss + 2 << ")" << m * 24 + (n * 6 + 3) << "    m=" <<
               m
                      << " n="<<n<< std::endl;*/
            points[i + m * 24 + (n * 6 + 3)] = 0.9f * ss;
            points[i + m * 24 + (n * 6 + 3) + 1] = 0.9f * ss;
            points[i + m * 24 + (n * 6 + 3) + 2] = 0.9f * ss;
          }
        }
        std::cout << ss << std::endl;
      }
      i += 96;
    }
    y += MSAA_STEPINT;
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

  //创建顶点着色器
  int vertexShader = glCreateShader(GL_VERTEX_SHADER);  
  glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
  glCompileShader(vertexShader);
  int success;
  char infoLog[512];
  glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);  //获取出错log
    std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
              << infoLog << std::endl;
  }

  //片段着色器
  int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER); 
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
  //顶点着色器和片段着色器都关联上着色器程序
  glAttachShader(shaderProgram, vertexShader);  
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

  float* vertices = new float[BUFF_ARR_SIZE];
  /*
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
  PaintLine(vertices, point_a4, point_b4, 30000, 90000);*/

  float point_a1[3] = { -1.0f, 0.0f, 0.0f };
  float point_b1[3] = { 0.0f, 1.0f, 0.0f };
  float point_c1[3] = { 1.0f, -1.0f, 0.0f };
  PaintTriangle(vertices, point_a1, point_b1, point_c1, 30000, 0);
  float point_a2[3] = { 0.0f, 1.0f, 0.0f };
  float point_b2[3] = {-0.5f, -1.0f, 0.0f};
  float point_c2[3] = {0.5f, -1.0f, 0.0f};
  PaintTriangleWithMSAA(vertices, point_a2, point_b2, point_c2, 30000, 2251002);

  //顶点数组对象：Vertex Array Object，VAO
  //顶点缓冲对象：Vertex Buffer Object，VBO
  unsigned int VBO, VAO;
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);
  glBindVertexArray(VAO);  //
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, BUFF_ARR_SIZE, vertices, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);



  //循环渲染
  while (!glfwWindowShouldClose(window)) {
    processInput(window);

    glClearColor(0.3f, 0.5f, 0.8f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shaderProgram);  //使用这个着色器程序
    glBindVertexArray(VAO);
    glDrawArrays(GL_POINTS, 0, BUFF_ARR_SIZE);  //绘制点

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