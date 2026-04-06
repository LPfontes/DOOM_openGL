#ifndef CAMERA_H
#define CAMERA_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

// Valores padrão da câmera
const float YAW         = -90.0f;
const float PITCH       =  0.0f;
const float SPEED       =  10.0f;
const float SENSITIVITY =  0.08f;
const float ZOOM        =  45.0f;

class Camera {
public:
    // Atributos da Câmera
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;
    // Ângulos de Euler
    float Yaw;
    float Pitch;
    // Opções da câmera
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;

    // construtor com vetores
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH) 
        : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM) {
        Position = position;
        WorldUp = up;
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors();
    }

    // retorna a matriz de visualização calculada usando os Ângulos de Euler e a Matriz LookAt
    glm::mat4 GetViewMatrix() {
        return glm::lookAt(Position, Position + Front, Up);
    }

    // processa a entrada recebida de qualquer sistema de entrada semelhante ao teclado. Aceita o parâmetro de entrada na forma de ENUM definido pela câmera (para abstraí-lo dos sistemas de janelas)
    void ProcessKeyboard(Camera_Movement direction, float deltaTime, bool constrainToGround = false) {
        float velocity = MovementSpeed * deltaTime;
        glm::vec3 moveFront = Front;
        if (constrainToGround) {
            moveFront.y = 0.0f;
            if (glm::length(moveFront) > 0.001f)
                moveFront = glm::normalize(moveFront);
        }

        if (direction == FORWARD)
            Position += moveFront * velocity;
        if (direction == BACKWARD)
            Position -= moveFront * velocity;
        if (direction == LEFT)
            Position -= Right * velocity;
        if (direction == RIGHT)
            Position += Right * velocity;
        if (direction == UP && !constrainToGround)
            Position += WorldUp * velocity;
        if (direction == DOWN && !constrainToGround)
            Position -= WorldUp * velocity;
    }

    // processa a entrada recebida de um sistema de entrada de mouse. Espera o valor de deslocamento (offset) tanto na direção x quanto na y.
    void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true) {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw   += xoffset;
        Pitch += yoffset;

        // certifique-se de que, quando o pitch (inclinação) estiver fora dos limites, a tela não seja invertida
        if (constrainPitch) {
            if (Pitch > 89.0f)
                Pitch = 89.0f;
            if (Pitch < -89.0f)
                Pitch = -89.0f;
        }

        // atualize os vetores Front, Right e Up usando os ângulos de Euler atualizados
        updateCameraVectors();
    }

    // processa a entrada recebida de um evento de roda de rolagem do mouse (scroll). Requer apenas entrada no eixo vertical da roda
    void ProcessMouseScroll(float yoffset) {
        Zoom -= (float)yoffset;
        if (Zoom < 1.0f)
            Zoom = 1.0f;
        if (Zoom > 45.0f)
            Zoom = 45.0f;
    }

private:
    // calcula o vetor frontal a partir dos Ângulos de Euler (atualizados) da Câmera
    void updateCameraVectors() {
        // calcula o novo vetor Front
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);
        // também recalcula o vetor Right e Up
        Right = glm::normalize(glm::cross(Front, WorldUp));  // normaliza os vetores, porque o comprimento deles fica mais próximo de 0 quanto mais você olha para cima ou para baixo, o que resulta em movimento mais lento.
        Up    = glm::normalize(glm::cross(Right, Front));
    }
};
#endif // CAMERA_H
