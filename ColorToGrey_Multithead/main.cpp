#include <iostream>
#include <fstream>
#include <string>
#include <string.h>
#include <algorithm>
#include <thread>

#define INICIO 0x0000
#define DIMENSIONES 0x0012
#define BPP 0x001C
#define DATA_OFFSET 0X000A

/// Comprueba si el archivo es de tipo .bmp
/// Los archivos .bmp tienen dos bytes al inicio del archivo con las letras 'BM', por lo
/// tanto comprobando estos dos primeros bytes podemos saber si es un archivo .bmp o no
///
/// param colorImage: referencia al archivo que se tiene que comprobar. Debe estar abierto
///                   en modo de lectura binaria
///
/// return un booleano que es verdadero si el archivo es de tipo .bmp
bool isBmpFile(std::ifstream &colorImage) {
    colorImage.seekg(INICIO);

    char fileType[3];
    colorImage.read(fileType, 2);
    fileType[2] = '\0';

    return strcmp(fileType, "BM") == 0;
}

/// Una vez confirmado que el archivo es .bmp, comprobamos si usa la cantidad de bpp
/// (bits per pixel) que soporta este programa, que en este caso son 24 bpp.
/// Esta información se almacena en 2 bytes en la posición 0x001A del archivo.
///
/// param colorImage: referencia al archivo que se tiene que comprobar. Debe estar abierto
///                   en modo de lectura binaria
///
/// return un booleano que es verdadero si el archivo es de 24 bpp
bool is24Bpp(std::ifstream &colorImage) {
    colorImage.seekg(BPP);

    int bpp = 0;
    colorImage.read((char *)&bpp, 2);

    return bpp == 24;
}

/// Obtenemos el número de filas y columnas de píxeles que tiene la imagen.
/// Esta información se almacena en 8 bytes, 4 para la anchura y 4 para la
/// altura, en la posición 0x0012 del archivo.
///
/// param colorImage: referencia al archivo que se tiene que comprobar. Debe estar abierto
///                   en modo de lectura binaria
/// param width: referencia a la variable que almacenará la anchura de la imagen, en píxeles
/// param height: referencia a la variable que almacenará la altura de la imagen, en píxeles
void getDimension(std::ifstream &colorImage, int &width, int &height) {
    colorImage.seekg(DIMENSIONES);

    colorImage.read((char *)&width, 4);
    colorImage.read((char *)&height, 4);
}

/// Copiamos toda la cabecera de un archivo .bmp en otro archivo. También almacenamos el tamaño
/// de la cabecera, en bytes.
///
/// param source: referencia al archivo del que se toma la información a copiar
/// param destiny: referencia al archivo donde se copia la información
/// param headerSize: referencia a la variable que almacenará la cantidad de bytes que ocupa
///                   la cabecera del archivo
void copyHeader(std::ifstream &source, std::ofstream &destiny, int &headerSize) {
    // El tamaño de la cabecera está almacenado en 4 bytes en la posición 0x000A del archivo
    source.seekg(DATA_OFFSET);
    headerSize = 0;
    source.read((char *)&headerSize, 4);

    source.seekg(INICIO); // Reseteamos el puntero en source para asegurarnor de que leemos la cabecera
    char c[headerSize];
    source.read(c, headerSize);
    destiny.write(c, headerSize);
}

/// Obtenemos el valor del gris correspondiente al RGB proporcionado
///
/// param r: la intensidad del rojo
/// param g: la intensidad del verde
/// param b: la intensidad del azul
///
/// return el valor del gris correspondiente
char grey(unsigned char r, unsigned char g, unsigned char b) {
    return 0.21 * r + 0.72 * g + 0.07 * b;
    //return (std::max(r, std::max(g, b)) + std::min(r, std::min(g, b))) / 2;
    //return (r + g + b) / 3;
}

/// Convierte los valores rgb de los píxeles al gris equivalente.
///
/// param pixelArray: referencia al array de bytes que contiene los valores rgb de cada pixel, ademas
///                   del offset que tenga cada línea
/// param height: cuantas líneas de la imagen contiene pixelArray
/// param width: cuantos píxeles tiene cada línea de píxeles
/// param lineOffset: la cantidad de bytes de relleno que tiene cada línea de pixeles al final
void rgbToGrey(unsigned char *pixelArray, int height, int width, int lineOffset) {
    int idx = 0;
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            // Los valores RGB están girados en el archivo, es decir, están en forma BGR
            char greyScale = grey(pixelArray[idx+2], pixelArray[idx+1], pixelArray[idx]);
            pixelArray[idx++] = greyScale;
            pixelArray[idx++] = greyScale;
            pixelArray[idx++] = greyScale;
        }
        idx += lineOffset;
    }
}

/// Coge un archivo .bmp y crea otro archivo idéntico pero cambiando la escala de colores
/// en una escala de grises mediante multithreading.
///
/// param colorImage: referencia a la imagen original
/// param greyImage: referencia a la imagen modificada
void convertToGrey(std::ifstream &colorImage, std::ofstream &greyImage) {
    int dataOffset; // Almacena en que posición empieza la información sobre los píxeles
    copyHeader(colorImage, greyImage, dataOffset);

    int width = 0;
    int height = 0;
    getDimension(colorImage, width, height);
    // Almacena cuantos bytes de relleno hay al final de cada línea para que cada línea tenga
    // un número de bytes divisible por 4
    int lineOffset = ((width*3) % 4 > 0) ? 4 - ((width*3) % 4) : 0;

    // Dividimos la imagen en dos mitades, cortando la imagen horizontalmente
    int firstHalfHeight = (height/2) + (height%2);
    int firstHalfSize =  firstHalfHeight * (width*3 + lineOffset);
    unsigned char firstHalf[firstHalfSize];

    int secondHalfHeight = (height/2);
    int secondHalfSize = secondHalfHeight * (width*3 + lineOffset);
    unsigned char secondHalf[secondHalfSize];

    // Rellenamos cada mitad
    colorImage.seekg(dataOffset);
    colorImage.read((char *)firstHalf, firstHalfSize);
    colorImage.read((char *)secondHalf, secondHalfSize);

    // Usamos multithreading para que se transforme cada mitad a gris de forma simultanea
    std::thread t1 = std::thread(rgbToGrey, &firstHalf[0], firstHalfHeight, width, lineOffset);
    std::thread t2 = std::thread(rgbToGrey, &secondHalf[0], secondHalfHeight, width, lineOffset);

    // Esperamos a que ambas mitades se hayan transformado
    t1.join();
    t2.join();

    // Introducimos la información de los píxeles en el achivo final
    greyImage.write((char *)firstHalf, firstHalfSize);
    greyImage.write((char *)secondHalf, secondHalfSize);

    std::cout << "La imagen se ha pasado a gris" << std::endl;
}

/// Este programa coge un archivo de tipo .bmp con 24 bpp y lo pasa a una escala de grises.
/// Tiene que indicarse como parámetro por línea de comandos la ruta al archivo .bmp
/// que se quiera transformar.
int main(int argc, char* argv[]) {
    std::ifstream colorImage; // La imagen original
    std::ofstream greyImage; // La imagen final

    if (argc >= 2) { // Comprobamos que se ha introducido un parametro por línea de cmandos
        colorImage.open(argv[1], std::ios::binary);

        if (isBmpFile(colorImage) && is24Bpp(colorImage)) {
            std::string filename = "grey_" + std::string(argv[1]);
            greyImage.open(filename.c_str(), std::ios::binary|std::ios::app);

            convertToGrey(colorImage, greyImage);

            greyImage.close();
        } else {
            std::cout << "Solo se aceptan archivos bmp de 24 bpp" << std::endl;
        }
    } else {
        std::cout << "Introduce la ruta del archivo bmp como parámetro" << std::endl;
    }

    colorImage.close();
}
