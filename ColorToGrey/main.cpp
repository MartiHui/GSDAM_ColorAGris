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

/// Coge un archivo .bmp y crea otro archivo idéntico pero cambiando la escala de colores
/// en una escala de grises de forma secuencial.
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

    colorImage.seekg(dataOffset); // Situamos el puntero al principio de la información de los píxeles
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
             unsigned char r = 0; // Unsigned, para tener valores del 0 al 255 en vez del -128 al 127
             unsigned char g = 0; // y evitar complicaciones a la hora de calcular el valor del gris
             unsigned char b = 0; // correspondiente

             // Los valores RGB se almacenan girados, es decir, el primer byte es el azul, el siguiente
             // es el verde y el último, el rojo, BGR.
             colorImage.read((char *)&b, 1);
             colorImage.read((char *)&g, 1);
             colorImage.read((char *)&r, 1);

             char greyScale = grey(r, g, b);
             greyImage << greyScale << greyScale << greyScale;
        }
        // Rellenamos la línea para que el número de bytes sea múltiplo de 4
        if (lineOffset > 0) {
            char c[lineOffset];
            colorImage.read(c, lineOffset);
            greyImage.write(c, lineOffset);
        }
    }

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
