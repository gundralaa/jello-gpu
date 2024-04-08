#include <utils.hpp>

const char *GLErrorStr(GLenum err)
{
    switch (err)
    {
    case GL_NO_ERROR:
        return "No error";
    case GL_INVALID_ENUM:
        return "Invalid enum";
    case GL_INVALID_VALUE:
        return "Invalid value";
    case GL_INVALID_OPERATION:
        return "Invalid operation";
    case GL_STACK_OVERFLOW:
        return "Stack overflow";
    case GL_STACK_UNDERFLOW:
        return "Stack underflow";
    case GL_OUT_OF_MEMORY:
        return "Out of memory";
    default:
        return "Unknown error";
    }
}

void getErrors(const char *place)
{
    GLenum error = glGetError();
    if (error)
    {
        printf("GL Error(s) at %s: \n", place);
        do
        {
            printf("\t%s\n", GLErrorStr(error));
        } while ((error = glGetError()));
    }
}

void getInfoLog(GLuint program)
{
    GLint InfoLogLength;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0)
    {
        std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
        glGetProgramInfoLog(program, InfoLogLength, NULL, &ProgramErrorMessage[0]);
        printf("%s\n", &ProgramErrorMessage[0]);
    }
}

void validateProgram(GLuint program)
{
    GLint buffer;
    glValidateProgram(program);
    glGetProgramiv(program, GL_VALIDATE_STATUS, &buffer);
    printf("%i", buffer);
    getInfoLog(program);
}

void loadShader(const char *path, GLuint type, GLuint program, char *prelude)
{
    // Create the shader
    GLuint ID = glCreateShader(type);

    // Read the Shader code from the file
    std::string code{prelude};
    std::ifstream stream(path, std::ios::in);
    if (stream.is_open())
    {
        std::stringstream sstr;
        sstr << stream.rdbuf();
        code.append(sstr.str());
        stream.close();
    }
    else
    {
        printf("Unable to open %s.\n", path);
        getchar();
        return;
    }

    GLint Result = GL_FALSE;
    int InfoLogLength;

    // Compile Shader
    printf("Compiling shader : %s\n", path);
    char const *SourcePointer = code.c_str();
    glShaderSource(ID, 1, &SourcePointer, NULL);
    glCompileShader(ID);

    // Check Shader
    glGetShaderiv(ID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(ID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0)
    {
        std::vector<char> ErrorMessage(InfoLogLength + 1);
        glGetShaderInfoLog(ID, InfoLogLength, NULL, &ErrorMessage[0]);
        printf("%s\n", &ErrorMessage[0]);
    }

    glAttachShader(program, ID);

    glGetProgramiv(program, GL_LINK_STATUS, &Result);
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0)
    {
        std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
        glGetProgramInfoLog(program, InfoLogLength, NULL, &ProgramErrorMessage[0]);
        printf("%s\n", &ProgramErrorMessage[0]);
    }
}