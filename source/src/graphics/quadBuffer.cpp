#include "quadBuffer.hpp"

#include "shader.hpp"
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <qopengl.h>

const std::array<QuadVertex, 6> QuadBuffer::quadVertices
{{ {{-1.0f, -1.0f}, {0.0f, 0.0f}},
      {{-1.0f, 1.0f},  {0.0f, 1.0f}},
        {{1.0f, -1.0f},  {1.0f, 0.0f}},
    
          {{-1.0f, 1.0f},  {0.0f, 1.0f}},
            {{1.0f, 1.0f},   {1.0f, 1.0f}},
              {{1.0f, -1.0f},  {1.0f, 0.0f}} }};

QuadBuffer::QuadBuffer()
{ }

QuadBuffer::~QuadBuffer()
{ }

bool QuadBuffer::initialized() const
{ return mInitialized; }

// make sure to call this from the OpenGL thread!
bool QuadBuffer::initGL(Shader *shader)
{
  if(!mInitialized)
    {
      initializeOpenGLFunctions();
      
      mVbo = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
      if(!mVbo->create())
        {
          LOGE("VBO create failed in QuadBuffer!!");
          delete mVbo;
          mVbo = nullptr;
          return false;
        }
      mVbo->setUsagePattern(QOpenGLBuffer::StaticDraw);

      mVao = new QOpenGLVertexArrayObject();
      if(!mVao->create())
	{
	  LOGE("VAO create failed in QuadBuffer!!");
          delete mVao;
          mVao = nullptr;
          mVbo->destroy();
          delete mVbo;
          mVbo = nullptr;
	  return false;
	}
      
      mVao->bind();
      mVbo->bind();
      shader->setAttrBuffer(0, GL_FLOAT, 0, 2, sizeof(QuadVertex) );
      shader->setAttrBuffer(1, GL_FLOAT, 2 * sizeof(float), 2, sizeof(QuadVertex));
      mVao->release();
      mVbo->allocate(quadVertices.data(), sizeof(QuadVertex)*quadVertices.size());
      mInitialized = true;
    }
  return true;
}

void QuadBuffer::cleanupGL()
{
  if(mInitialized)
    {
      mVao->destroy();
      mVbo->destroy();
      delete mVbo;
      delete mVao;
      mInitialized = false;
    }
}

void QuadBuffer::render()
{
  mVao->bind();
  glDrawArrays(GL_TRIANGLES, 0, quadVertices.size());
  mVao->release();
}
