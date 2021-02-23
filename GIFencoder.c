/*----------------------------------
FILIPE GOOD - 2015239132
FLAVIO PEREIRA - 2015228673
HENRIQUE BRANQUINHO - 2015239873
2016/2017 - TI PL3 - TP2
---------------------------------*/



#include "GIFencoder.h"

#include "math.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"





// conversão de um objecto do tipo Image numa imagem indexada
imageStruct* GIFEncoder(unsigned char *data, int width, int height) {
	
	imageStruct* image = (imageStruct*)malloc(sizeof(imageStruct));
	image->width = width;
	image->height = height;

	//converter para imagem indexada
	RGB2Indexed(data, image);

	return image;
}
		
//conversão de lista RGB para indexada: máximo de 256 cores
void RGB2Indexed(unsigned char *data, imageStruct* image) {
	int x, y, colorIndex, colorNum = 0;
	char *copy;

	image->pixels = (char*)calloc(image->width*image->height, sizeof(char));
	image->colors = (char*)calloc(MAX_COLORS * 3, sizeof(char));
	
	
	for (x = 0; x < image->width; x++) {
		for (y = 0; y < image->height; y++) {
			for (colorIndex = 0; colorIndex < colorNum; colorIndex++) {
				if (image->colors[colorIndex * 3] == (char)data[(y * image->width + x)*3] && 
					image->colors[colorIndex * 3 + 1] == (char)data[(y * image->width + x)*3 + 1] &&
					image->colors[colorIndex * 3 + 2] == (char)data[(y * image->width + x)*3 + 2])
					break;
			}

			if (colorIndex >= MAX_COLORS) {
				printf("Demasiadas cores...\n");
				exit(1);
			}

			image->pixels[y * image->width + x] = (char)colorIndex;

			if (colorIndex == colorNum) 
			{
				image->colors[colorIndex * 3]	  = (char)data[(y * image->width + x)*3];
				image->colors[colorIndex * 3 + 1] = (char)data[(y * image->width + x)*3 + 1];
				image->colors[colorIndex * 3 + 2] = (char)data[(y * image->width + x)*3 + 2];
				colorNum++;
			}
		}
	}

	//define o número de cores como potência de 2 (devido aos requistos da Global Color Table)
	image->numColors = nextPower2(colorNum);

	//refine o array de cores com base no número final obtido
	copy = (char*)calloc(image->numColors*3, sizeof(char));
	memset(copy, 0, sizeof(char)*image->numColors*3);
	memcpy(copy, image->colors, sizeof(char)*colorNum*3);
	image->colors = copy;

	image->minCodeSize = numBits(image->numColors - 1);
	if (image->minCodeSize == (char)1)  //imagens binárias --> caso especial (pág. 26 do RFC)
		image->minCodeSize++;
}
	
		
//determinação da próxima potência de 2 de um dado inteiro n
int nextPower2(int n) {
	int ret = 1, nIni = n;
	
	if (n == 0)
		return 0;
	
	while (n != 0) {
		ret *= 2;
		n /= 2;
	}
	
	if (ret % nIni == 0)
		ret = nIni;
	
	return ret;
}
	
	
//número de bits necessário para representar n
char numBits(int n) {
	char nb = 0;
	
	if (n == 0)
		return 0;
	
	while (n != 0) {
		nb++;
		n /= 2;
	}
	
	return nb;
}


//---- Função para escrever imagem no formato GIF, versão 87a
//// COMPLETAR ESTA FUNÇÃO
void GIFEncoderWrite(imageStruct* image, char* outputFile) {
	
	FILE* file = fopen(outputFile, "wb");
	char trailer;

	//Escrever cabeçalho do GIF
	writeGIFHeader(image, file);
	
	//Escrever cabeçalho do Image Block
	// CRIAR FUN‚ÌO para ESCRITA do IMAGE BLOCK HEADER!!!
	//Sugest‹o da assinatura do mŽtodo a chamar:
	//
	writeImageBlockHeader(image, file);
	
	/////////////////////////////////////////
	//Escrever blocos com 256 bytes no m‡ximo
	/////////////////////////////////////////
	//CODIFICADOR LZW AQUI !!!! 
	//Sugest‹o de assinatura do mŽtodo a chamar:
	//
	
	LZWCompress(file, image->minCodeSize, image->pixels, image->width*image->height);
	
	
	fprintf(file, "%c", (char)0);  //Block terminator
	
	//trailer
	trailer = 0x3b;
	fprintf(file, "%c", trailer);
	
	fclose(file);
}
	
	
//--------------------------------------------------
//gravar cabeçalho do GIF (até global color table)
void writeGIFHeader(imageStruct* image, FILE* file) {

	int i;
	char toWrite, GCTF, colorRes, SF, sz, bgci, par;

	//Assinatura e versão (GIF87a)
	char* s = "GIF87a";
	for (i = 0; i < (int)strlen(s); i++)
		fprintf(file, "%c", s[i]);	

	//Ecrã lógico (igual à da dimensão da imagem) --> primeiro o LSB e depois o MSB
	fprintf(file, "%c", (char)( image->width & 0xFF));
	fprintf(file, "%c", (char)((image->width >> 8) & 0xFF));
	fprintf(file, "%c", (char)( image->height & 0xFF));
	fprintf(file, "%c", (char)((image->height >> 8) & 0xFF));
	
	//GCTF, Color Res, SF, size of GCT
	GCTF = 1;
	colorRes = 7;  //número de bits por cor primária (-1)
	SF = 0;
	sz = numBits(image->numColors - 1) - 1; //-1: 0 --> 2^1, 7 --> 2^8
	toWrite = (char) (GCTF << 7 | colorRes << 4 | SF << 3 | sz);
	fprintf(file, "%c", toWrite);

	//Background color index
	bgci = 0;
	fprintf(file, "%c", bgci);

	//Pixel aspect ratio
	par = 0; // 0 --> informação sobre aspect ratio não fornecida --> decoder usa valores por omissão
	fprintf(file, "%c",par);

	//Global color table
	for (i = 0; i < image->numColors * 3; i++)
		fprintf(file, "%c", image->colors[i]);
}

void writeImageBlockHeader(imageStruct * image, FILE * file){

	char imageSeparator, LCTF, interlace, SF, reserved, sizeLCTF, toWrite;
	int top, left;

	imageSeparator = 0x2c;
	fprintf(file, "%c", imageSeparator);

	 //Image Left e Top Position
	left = 0;
	top = 0;
	fprintf(file, "%c", (char) (left & 0xFF));
	fprintf(file, "%c", (char) ((left >> 8) & 0xFF));
	fprintf(file, "%c", (char) (top & 0xFF));
	fprintf(file, "%c", (char) ((top >> 8) & 0xFF));

	fprintf(file, "%c", (char)( image->width & 0xFF));
	fprintf(file, "%c", (char)((image->width >> 8) & 0xFF));
	fprintf(file, "%c", (char)( image->height & 0xFF));
	fprintf(file, "%c", (char)((image->height >> 8) & 0xFF));   // Image width e height, igual ao ecrã lógico

	LCTF = 0; //Local color table
	interlace = 0; //Interlace
	SF = 0; //LCT sort
	reserved = 0; //REserved
	sizeLCTF = 0; //Tamanho da LCT

	toWrite = (char) (LCTF << 7 | interlace << 6 | SF << 5 | reserved << 3 | sizeLCTF);
	fprintf(file, "%c", toWrite);

	fprintf(file, "%c", (char)( image->minCodeSize)); //LZW Min Code Size

}

void LZWCompress(FILE * file, char minCodeSize, char * pixels, int npixel){
	Dict * dict;
	int dictpos,pixelpos,findex1,findex2; //pixelpos para controlar a posiçao em pixels, dictpos posição do dicionário
	//findex - find index  numbits = numbits atuais para codificar LZW
	int numbits, freebits,blockindex,blocksize;  //PARA WRITE CODE
	int clearcode, endofinfo;
	char temp[4096];
	char * newc;
	char subblock[255];
	

	pixelpos = 0;
	strcpy(temp,"");

	freebits = 8;
	blockindex = 0;
	blocksize = 255;



	numbits = minCodeSize + 1;

	clearcode = pow(2,(int)minCodeSize);
	endofinfo = clearcode + 1;


	dict = initDict(minCodeSize,&dictpos,clearcode,endofinfo); //CRIA O DICIONARIO INICIALIZADO

	

	//INICIALIZA BUFFER A 0's
	emptyBuffer(subblock,&blockindex);

	//ESCREVER CLEAR CODE NO PRIMEIRO SUB BLOCO
	writeCode(file,subblock,clearcode,numbits,&freebits,&blockindex,blocksize);

	while(pixelpos < npixel){

		newc = intToChar((int)pixels[pixelpos]); //Lê a nova informação do novo píxel


		if(strlen(temp)) strcat(temp,","); //Se temp não estiver vazio, acrescenta "," 
		strcat(temp,newc);  //Acrescenta a temp o que foi lido (newc)
		

		findex1 = findDict(dict,temp,dictpos);	 //Procura no dicionario por temp para ver se exite

		

		if(findex1 == -1){ //Se nao existir no dicionario
			 
			
			writeCode(file,subblock,findex2,numbits,&freebits,&blockindex,blocksize);
			//Escrevemos o codigo anterior (findex2) 

			
			
			addDict(dict,dictpos,temp);
			//Adicionamos ao dicionario

			dictpos++; 
		
 			
			
			
			if(dictpos == 4096){
				//SE o dicionario estiver cheio, fazemos reset
				resetDict(dict,endofinfo+1,dictpos);
				//Temos de escrever o clearcode
				writeCode(file,subblock,clearcode,numbits,&freebits,&blockindex,blocksize);
				//numbits e dictpos voltam ao valor inicial
				numbits = minCodeSize + 1;				
				dictpos = endofinfo + 1;
			}
			
			if(dictpos-1 == nextPower2(dictpos-1)){
				//Sempre que a posição do dicionário atingir uma potência de 2, temos de aumentar o numbits de escrita
				numbits++;
			} 
			
			//Esvaziamos temp 
			strcpy(temp,"");
			
		}

		else{ //Se encontrar no dicionário o que foi lido

			findex2 = findex1; //Guardamos o índice do que foi encontrado para quando for preciso escrever
			
			pixelpos++;
		}
		
		free(newc); //Libertar newc
		


	} //WHILE

	//APESAR DE PIXELPOS TER ULTRAPASSADO O LIMITE, AINDA TEMOS VALORES POR ESCREVER:
	findex1 = findDict(dict,temp,dictpos);	
	writeCode(file,subblock,findex1,numbits,&freebits,&blockindex,blocksize);

	//ESCREVE O END OF INFORMATION
	writeCode(file,subblock,endofinfo,numbits,&freebits,&blockindex,blocksize);

	//VERIFICAR SE É NECESSÁRIO ESCREVER SUBBLOCK OU SE JÁ FOI ESCRITO
	if (!(freebits == 8 && blockindex == 0)){
		//VERIFICAR ATÉ ONDE TEMOS DE ESCREVER NO FICHEIRO (QUESTAO DE INDICES APENAS)
		if(freebits == 8)
			writeToFile(file,subblock,blockindex);
		else writeToFile(file,subblock,blockindex+1);
	}
	//LIBERTAMOS A MEMORIA DO DICIONARIO
	resetDict(dict,0,dictpos);
	free(dict);

	

} 





Dict * initDict(char minCodSize, int * finalpos, int clearcode, int endofinfo){
	Dict * dict;
	int i,ncolors;
	int dictsize = pow(2,12); //MAXIMO TAMANHO = 4096
	dict = (Dict * ) malloc( 2 * dictsize * sizeof(Dict)); //2* PARA PREVENIR ERROS DE MALLOC
	ncolors = pow(2,minCodSize); //NCOLORS = 2^MINCODESIZE
	for (i = 0; i < ncolors; i++){ //INICIALIZA TODAS AS ENTRADAS COM OS INDICES POSSIVEIS
		dict[i].index = i;
		dict[i].key = intToChar(i);
		
	}

	//CLEAR CODE
	dict[i].index = i;
	dict[i].key = intToChar(clearcode);
	
	i++;
	//EOI
	dict[i].index = i;
	dict[i].key = intToChar(endofinfo);

	i++;
	(*finalpos) = i; //GUARDA A POSIÇÃO DO DICIONÁRIO ONDE PODEMOS ACRESCENTAR ENTRADAS

	return dict;
}




char * intToChar(int n){ 
	//FUNÇÃO QUE TRANSFORMA UM NÚMERO INTEIRO NUMA STRING
	int nchars;
	char * res;
	nchars = numDigs(n); //CALCULA O NÚMERO DE DIGITOS DO NÚMERO
	res = (char *) malloc(nchars * sizeof(char)); //ALOCA O TAMANHO NECESSÁRIO (2* para corrigir error malloc() too fast)
	sprintf(res,"%d",n); //CONVERTE PARA STRING
	return res;
}

int numDigs(int n){ //SEMELHANTE A numBits MAS PARA DESCOBRIR O NUMERO DE DIGITOS
	int res = 0;
	if ( n == 0) return 1;
	while(n != 0){
		n /= 10;
		res++;
	}
	return res;
}

int findDict(Dict * dict, char * target, int dictpos){
	//FUNÇÃO PARA PROCURAR VALORES NO DICIONARIO
	//DEVOLVE -1 SE NAO ENCONTRAR 
	int i;
	for(i = 0; i < dictpos; i++){
		if(strcmp(dict[i].key,target) == 0) {
			return dict[i].index;
		}
	}
	return -1;
}

void addDict(Dict * dict, int dictpos, char * new){
	dict[dictpos].index = dictpos;
	dict[dictpos].key = (char *) malloc(2 * strlen(new) * sizeof(char)); //ERROS DE MALLOC
	strcpy(dict[dictpos].key,new);
}


void writeCode(FILE * file, char * buffer, int code, int numbits, int * freebits, int * blockindex, int blocksize){
	int written = 0; //VARIAVEL QUE GUARDA O NUMERO DE BITS QUE JA FORAM ESCRITOS
	int aux = code; 

	
	while(written != numbits){

		buffer[*blockindex] |= aux << (8 - (*freebits));
		//BUFFER[INDEX] = BUFFER[INDEX] | AUX << (8 - FREEBITS)
		//como buffer[index] = 0 inicialmente, deslocamos o que queremos escrever para a esquerda 
		//até onde não houver informação, fazemos OR para escrever ai a nova informação sem
		//alterar a que já estava no byte em questão

		if((*freebits) >= numbits - written){
			//Se freebits for maior que a diferença entre o que temos de escrever e o que falta escrever, 
			//quer dizer que temos espaço suficiente para escrever o resto da informação que falta (numbits - written) 

			(*freebits) -= (numbits - written);
			written = numbits; //written = numbits, escrevemos tudo 
			

		}

		else{
			//se não houver espaço suficiente, escrevemos apenas um número de bits
			//igual a freebits, e deslocamos o número (aux) para a direita com um deslocamento igual ao que foi escrito(freebits)
			written += *freebits;
			aux = aux >> (*freebits);
			(*freebits) = 0;

		}

		if(*freebits == 0){
			
			(*blockindex)++;

			if((*blockindex) == blocksize){
				//O bloco encheu, escrevemos para o ficheiro e esvaziamos o buffer
				writeToFile(file,buffer,blocksize);
				emptyBuffer(buffer,blockindex);
			}

			(*freebits) = 8;

		}

		
	}


}

void writeToFile(FILE * file, char * buffer, int blocksize){
	int i;
	fprintf(file,"%c",blocksize);
	
	for(i = 0; i < blocksize; i++){
		
		fprintf(file, "%c",buffer[i]);
	}
}

void emptyBuffer(char * buffer, int * blockindex){
	int i;
	for(i = 0; i < 255; i++)
		buffer[i] = 0;
	(*blockindex) = 0;
}

void resetDict(Dict * dict, int initial, int dictpos){
	int i;
	for(i = initial; i < dictpos; i++){
		if(dict[i].key != NULL) free(dict[i].key);
	}
}
