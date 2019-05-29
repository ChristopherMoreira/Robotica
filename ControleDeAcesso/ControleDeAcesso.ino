#include <EEPROM.h>     // Vamos ler e escrever os UID's na EEPROM
#include <SPI.h>         
#include <MFRC522.h>  

#define LED_COMUM

//PERDIDONA NESSES IFDEF E ELSEDEF, mas deve ser porque algum momento inverte as coisas

#ifdef LED_COMUM
#define LED_ON LOW
#define LED_OFF HIGH
#else
#define LED_ON HIGH
#define LED_OFF LOW
#endif

#define led_vermelho 7    // Define pin dos leds
#define led_verde 6
#define led_amarelo 5

#define rele 4     // Defino pin do rele
#define botao 3     // Define pin do botao de reset

bool programa = false;  // Inicializa o programa como falso

uint8_t sucessoLer;    // Variavel inteira para mantar caso tenha uma leitura bem sucedida do leitor rfid

byte armazenacartao[4];   // Armazena um id lido no EEPROM
byte lercartao[4];   // Le o id dos cartoes armazenados no EEPROM
byte cartaomestre[4];   // Armazena o id do cartao master na EEPROM

// Cria instancia MFRC522.
#define SS_PIN 10
#define RST_PIN 9
MFRC522 mfrc522(SS_PIN, RST_PIN);

// -------------------------------------- Configuracoes --------------------------------------------------
void setup() {
  //Configuracao dos pin do arduino
  pinMode(led_vermelho, OUTPUT);
  pinMode(led_verde, OUTPUT);
  pinMode(led_amarelo, OUTPUT);
  pinMode(botao, INPUT_PULLUP);   // Habilita o resistor pull up
  pinMode(rele, OUTPUT);
  
  digitalWrite(rele, LOW);    // Certifica que a trava solenoide esta fechada
  digitalWrite(led_vermelho, LED_OFF);  // Certifica que o led vermelho esta desligado
  digitalWrite(led_verde, LED_OFF);  // Certifica que o led verde esta desligado
  digitalWrite(led_amarelo, LED_OFF); // Certifica que o led amarelo esta desligado

  // Configuracoes iniciais
  Serial.begin(9600);  // Inicializa a comunicacao serial com o PC, definindo a taxa de transferencia em 9600 bits
  SPI.begin();           // MFRC522 Hardware usa o protocolo SPI
  mfrc522.PCD_Init();    // Inicializae MFRC522 Hardware

  //Se defirnirmos Antenna Gain to Max vai fazer com que leia a uma distancia maior, mas nao sera necessario na sala de aula
  //mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max); codigo pronto caso necessite

//  Serial.println(F("Exemplo de Controle de Acesso v0.1"));   // Para fins de depuração
  ShowReaderDetails(); 

  
  if (digitalRead(botao) == LOW) {  
    digitalWrite(led_vermelho, LED_ON); // O led vermelho permanecera ligado para demonstrar que esta ocorrendo o reset
    Serial.println(F("O botao de reset foi pressionado"));
    Serial.println(F("Voce tem 10 segundos para cancelar o reset"));
    Serial.println(F("Isto removera todos os registros e nao podera ser revertido"));
    bool buttonState = monitorbotaoutton(10000); // Tempo para o usuario cancelar o reset
    if (buttonState == true && digitalRead(botao) == LOW) {    // Se o botao ainda estiver pressionado, reset a EEPROM
      Serial.println(F("Iniciando o reset da EEPROM"));
      for (uint16_t x = 0; x < EEPROM.length(); x = x + 1) {    //Loop até o fim dos endereços da EEPROM
        if (EEPROM.read(x) == 0) {              //Se o endereco da EEPROM for 0
          //Nao faça nada, va para o proximo endereço, para economizar tempo
        }
        else {
          EEPROM.write(x, 0);       // se nao precisar escrever 0 para limpar, levara um total de 3.3mS
        }
      }
      Serial.println(F("EEPROM Resetada com Sucesso"));
      digitalWrite(led_vermelho, LED_OFF);  // Mostra atraves dos leds que foi resetado com sucesso
      delay(200);
      digitalWrite(led_vermelho, LED_ON);
      delay(200);
      digitalWrite(led_vermelho, LED_OFF);
      delay(200);
      digitalWrite(led_vermelho, LED_ON);
      delay(200);
      digitalWrite(led_vermelho, LED_OFF);
    }
    else {
      Serial.println(F("O reset foi cancelado")); // Feedback para informar que o reset foi cancelado pelo serial e atraves de led
      digitalWrite(led_vermelho, LED_OFF);
    }
  }
  // Procura por um ID master, se ainda nao foi definido da a oportunidade ao usuario de definir
  if (EEPROM.read(1) != 143) {
    Serial.println(F("Cartao RFID Master ainda nao foi definido"));
    Serial.println(F("Procurando por PICC para ser definido como cartao master"));
    do {
      sucessoLer = getID();            // Define que a leitura foi bem sucessida para 1, se nao a deixa em 0
      digitalWrite(led_amarelo, LED_ON);    // mostra nos leds que o cartao master ainda precisa ser definido
      delay(200);
      digitalWrite(led_amarelo, LED_OFF);
      delay(200);
    }
    while (!sucessoLer);                  // Enquanto o cartao master nao for lido com sucesso o programa nao vai continuar
    for ( uint8_t j = 0; j < 4; j++ ) {        // Loop 4 times
      EEPROM.write( 2 + j, lercartao[j] );  // Escreva o PICC UID escaneado no EEPROM, comecando do 3 endereco
    }
    EEPROM.write(1, 143);                  // Escreva na EEPROM definindo o cartao master
    Serial.println(F("Cartao Master foi definido com sucesso!"));
  }
//  Serial.println(F("-------------------"));
//  Serial.println(F("Master Card's UID"));
//  for ( uint8_t i = 0; i < 4; i++ ) {          // Leia o UID do cartao master da EEPROM
//   cartaomestre[i] = EEPROM.read(2 + i);    // Escreva o UID do cartao master
//    Serial.print(cartaomestre[i], HEX);
//  }
  Serial.println("");
  Serial.println(F("-------------------"));
  Serial.println(F("Tudo esta pronto!"));
  Serial.println(F("Procurando PICCs para serem lidos"));
  cycleLeds();    // Feedback de tudo pronto atraves dos leds
}


///////////////////////////////////////// Funcao Main ///////////////////////////////////
void loop () {
  do {
    sucessoLer = getID();  
    // Se o butao for pressionado por 10 segundos enquanto o dispositivo estiver ligado tudo sera resetado
    if (digitalRead(botao) == LOW) { // Olha se o butao foi pressionado
      // Mostra que o botao de reset foi pressionado interrompento todas as outras atividades do dispositivo
      digitalWrite(led_vermelho, LED_ON);  
      digitalWrite(led_verde, LED_OFF);  
      digitalWrite(led_amarelo, LED_OFF);
      // Mostra no serial que o botao foi pressionado
      Serial.println(F("O botao de reset foi pressionado"));
      Serial.println(F("O cartao master sera apagado! Em 10 segundos"));
      bool buttonState = monitorbotaoutton(10000); // Tempo para cancelar o reset
      if (buttonState == true && digitalRead(botao) == LOW) {    // Se o botao estiver pressionado, reseta EEPROM
        EEPROM.write(1, 0);                  // Numero para resetar
        Serial.println(F("O cartao master foi apagado do dispositivo"));
        Serial.println(F("Por favor, reinicie para reprogramar um cartao master"));
        while (1);
      }
      Serial.println(F("Reset de cartao master foi cancelado"));
    }
    if (programa) {
      cycleLeds();              // Mostra atraves dos leds que esta procurando um novo cartao
    }
    else {
      normalModeOn();     // Define os leds para o modo normal, somente o amarelo ligado e os outros desligados
    }
  }
  while (!sucessoLer);   // Enquanto o cartao nao for lido com sucesso o programa nao vai continuar
  if (programa) {
    if ( isMaster(lercartao) ) { // Enquanto no modo de programador, verifica se o cartao master ja foi ativo, se sim finaliza o modo de programador
      Serial.println(F("Cartao Master foi lido"));
      Serial.println(F("Saindo do modo de programador"));
      Serial.println(F("-----------------------------"));
      programa = false;
      return;
    }
    else {
      if ( findID(lercartao) ) { // Se o cartao ja foi adicionado ao banco de dados, exclua-o
        Serial.println(F("Eu conheço este PICC, removendo-o..."));
        deleteID(lercartao);
        Serial.println("-----------------------------");
        Serial.println(F("Procurando PICC para adicionar ou remover da EEPROM"));
      }
      else {                    // Se o cartao ainda nao foi adicionado, adicione-o a lista
        Serial.println(F("Eu não conheço este PICC, adicionando-o..."));
        writeID(lercartao);
        Serial.println(F("-----------------------------"));
        Serial.println(F("Procurando PICC para adicionar ou remover da EEPROM"));
      }
    }
  }
  else {
    if ( isMaster(lercartao)) {    // Se o cartao lido for correspondente ao cartao master, entre no modo programador
      programa = true;
      Serial.println(F("Ola Master - Entrando no modo de programador"));
      uint8_t count = EEPROM.read(0);   // Leia o primeiro Byte da EEPROM
      Serial.print(F("Eu tenho "));     // diga quantos ID's tem na EEPROM
      Serial.print(count);
      Serial.print(F(" gravado(s) na EEPROM"));
      Serial.println("");
      Serial.println(F("Procurando PICC para adicionar ou remover da EEPROM"));
      Serial.println(F("Leia o cartao master novamente para sair do modo de programador"));
      Serial.println(F("-----------------------------"));
    }
    else {
      if ( findID(lercartao) ) { // Procure pelo cartao na EEPROM
        Serial.println(F("Bem vindo, você esta autorizado a passar"));
        granted(300);         // abre a porta por 300 ms
      }
      else {      // Se nao, mostre que o ID nao era um ID valido
        Serial.println(F("Voce nao esta autorizado a passar"));
        denied();
      }
    }
  }
}

/////////////////////////////////////////  Acesso Concedido   ///////////////////////////////////
void granted ( uint16_t setDelay) {
  digitalWrite(led_amarelo, LED_OFF);   // 
  digitalWrite(led_vermelho, LED_OFF);  // 
  digitalWrite(led_verde, LED_ON);   // 
  digitalWrite(rele, HIGH);     // Abre a trava solenoide ativando o rele
  delay(setDelay);          // Deixa a trava aberta por alguns segundo
  digitalWrite(rele, LOW);    // Fecha a trava solenoide
  delay(1000);            // Deixa o led verde aceso por 1 segundo
}

///////////////////////////////////////// Acesso Negado  ///////////////////////////////////
void denied() {
  digitalWrite(led_verde, LED_OFF);  //Certifique-se de que o LED verde esteja apagado
  digitalWrite(led_amarelo, LED_OFF);   // Certifique-se de que o LED amarelo esteja apagado
  digitalWrite(led_vermelho, LED_ON);   // Ligue o LED vermelho
  delay(1000);
}


///////////////////////////////////////// Obter o UID do PICC ///////////////////////////////////
uint8_t getID() {
  // Preparando-se para ler PICCs
  if ( ! mfrc522.PICC_IsNewCardPresent()) { //Se um novo PICC colocado no leitor RFID continue
    return 0;
  }
  if ( ! mfrc522.PICC_ReadCardSerial()) {   //Desde que um PICC colocado obtenha o Serial e continue
    return 0;
  }
  Serial.println(F("Scanneando PICC's UID:"));
  for ( uint8_t i = 0; i < 4; i++) {  //
    lercartao[i] = mfrc522.uid.uidByte[i];
    Serial.print(lercartao[i], HEX);
  }
  Serial.println("");
  mfrc522.PICC_HaltA(); // para de ler
  return 1;
}

void ShowReaderDetails() {
  // Obtenha a versão do software MFRC522
  byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  Serial.print(F("MFRC522 Software Version: 0x"));
  Serial.print(v, HEX);
  if (v == 0x91)
    Serial.print(F(" = v1.0"));
  else if (v == 0x92)
    Serial.print(F(" = v2.0"));
  else
    Serial.print(F("(desconhecido),provavelmente um clone chinês"));
  Serial.println("");
  // Quando 0x00 ou 0xFF é retornado, a comunicação provavelmente falhou
  if ((v == 0x00) || (v == 0xFF)) {
    Serial.println(F("AVISO: Falha de comunicação, o MFRC522 está conectado corretamente?"));
    Serial.println(F("SISTEMA INTERROMPIDO: Verifique as conexões."));
    // Visualize system is halted
    digitalWrite(led_verde, LED_OFF);  // Verifique se o LED verde está apagado
    digitalWrite(led_amarelo, LED_OFF);   // Verifique se o LED amarelo está apagado
    digitalWrite(led_vermelho, LED_ON);   //Ligue o LED vermelho
    while (true); // não vá mais longe
  }
}

///////////////////////////////////////// Leds de Ciclo (Modo de Programação)///////////////////////////////////
void cycleLeds() {
  digitalWrite(led_vermelho, LED_OFF);  // Certifique-se de que o LED vermelho está apagado
  digitalWrite(led_verde, LED_ON);   // Certifique-se de que o LED verde esteja aceso
  digitalWrite(led_amarelo, LED_OFF);   // Certifique-se de que o LED amarelo está apagado
  delay(200);
  digitalWrite(led_vermelho, LED_OFF);  // Certifique-se de que o LED vermelho está apagado
  digitalWrite(led_verde, LED_OFF);  // MCertifique-se de que o LED verde está apagado
  digitalWrite(led_amarelo, LED_ON);  //Certifique-se de que o LED amarelo esteja aceso
  delay(200);
  digitalWrite(led_vermelho, LED_ON);   // Certifique-se de que o LED vermelho esteja aceso
  digitalWrite(led_verde, LED_OFF);  // Certifique-se de que o LED verdeo está apagado
  digitalWrite(led_amarelo, LED_OFF);   // Certifique-se de que o LED amarelo está apagado
  delay(200);
}

//////////////////////////////////////// Modo Normal do LED  ///////////////////////////////////
void normalModeOn () {
  digitalWrite(led_amarelo, LED_ON);  // LED amarelo ligado e pronto para ler o cartão
  digitalWrite(led_vermelho, LED_OFF);  // Certifique-se de que o LED vermelho está apagado
  digitalWrite(led_verde, LED_OFF);  //Certifique-se de que o LED verde está apagado
  digitalWrite(rele, HIGH);    //Certifique-se de que a porta esteja travada  // RELE
}

//////////////////////////////////////// Ler um ID da EEPROM //////////////////////////////
void readID( uint8_t number ) {
  uint8_t start = (number * 4 ) + 2;    // Descobrir a posição inicial
  for ( uint8_t i = 0; i < 4; i++ ) {     // Loop 4 vezes para obter os 4 bytes
    armazenacartao[i] = EEPROM.read(start + i);   // Atribuir valores lidos da EEPROM para o array
  }
}

///////////////////////////////////////// Adicionar um ID à EEPROM   ///////////////////////////////////
void writeID( byte a[] ) {
  if ( !findID( a ) ) {     //Antes de escrevermos para a EEPROM, verifique se já vimos este cartão antes!
    uint8_t num = EEPROM.read(0);     // Obter o número de espaços usados, a posição 0 armazena o número de cartões de identificação
    uint8_t start = ( num * 4 ) + 6;  // Descubra onde o próximo slot começa
    num++;                // Incrementar o contador um por um
    EEPROM.write( 0, num );     // Escreva a nova contagem para o contador
    for ( uint8_t j = 0; j < 4; j++ ) {   // Loop 4 vezes
      EEPROM.write( start + j, a[j] );  //Escreva os valores da matriz para EEPROM na posição correta
    }
    successWrite();
    Serial.println(F("Registro de ID adicionado com sucesso à EEPROM"));
  }
  else {
    failedWrite();
    Serial.println(F("FALHA! Há algo errado com ID ou a EEPROM está ruim"));
  }
}

///////////////////////////////////////// Remover ID da EEPROM   ///////////////////////////////////
void deleteID( byte a[] ) {
  if ( !findID( a ) ) {     // Antes de excluirmos da EEPROM, verifique se temos este cartão!
    failedWrite();      // Se não
    Serial.println(F("FALHA! Há algo errado com ID ou a EEPROM está ruim"));
  }
  else {
    uint8_t num = EEPROM.read(0);   // Obter o número de espaços usados, a posição 0 armazena o número de cartões de identificação
    uint8_t slot;       // Descobrir o número do slot do cartão
    uint8_t start;      // = ( num * 4 ) + 6; // Descubra onde o próximo slot começa
    uint8_t looping;    // O número de vezes que o loop é repetido
    uint8_t j;
    uint8_t count = EEPROM.read(0); // Leia o primeiro Byte da EEPROM que armazena o número de cartões
    slot = findIDSLOT( a );   // Descobrir o número do slot do cartão para excluir
    start = (slot * 4) + 2;
    looping = ((num - slot) * 4);
    num--;      // Decrementar o contador um por um
    EEPROM.write( 0, num );   // Escreva a nova contagem para o contador
    for ( j = 0; j < looping; j++ ) {         //Loop o tempo de troca de cartão
      EEPROM.write( start + j, EEPROM.read(start + 4 + j));   // Mude os valores da matriz para 4 lugares antes na EEPROM
    }
    for ( uint8_t k = 0; k < 4; k++ ) {         // Loop de mudança
      EEPROM.write( start + j + k, 0);
    }
    successDelete();
    Serial.println(F("Registro de ID removido com sucesso da EEPROM"));
  }
}

///////////////////////////////////////// Verificar Bytes   ///////////////////////////////////
bool checkTwo ( byte a[], byte b[] ) {   
  for ( uint8_t k = 0; k < 4; k++ ) {   // Loop 4 vezes
    if ( a[k] != b[k] ) {     // Se a != b então falso, porque: um falha, tudo falha
       return false;
    }
  }
  return true;  
}

///////////////////////////////////////// Encontre o Slot   ///////////////////////////////////
uint8_t findIDSLOT( byte find[] ) {
  uint8_t count = EEPROM.read(0);       // Leia o primeiro byte da EEPROM q
  for ( uint8_t i = 1; i <= count; i++ ) {    //  Loop uma vez para cada entrada da EEPROM
    readID(i);                // Lê uma ID da EEPROM, ela é armazenada no armazenacartao[4]
    if ( checkTwo( find, armazenacartao ) ) {   //  Verifique se o cartão armazenado lê da EEPROM
      // é o mesmo que o cartão de identificação find [] passado
      return i;         // O número do slot do cartão
    }
  }
}

///////////////////////////////////////// Encontre o ID da EEPROM   ///////////////////////////////////
bool findID( byte find[] ) {
  uint8_t count = EEPROM.read(0);     // Leia o primeiro Byte da EEPROM 
  for ( uint8_t i = 1; i < count; i++ ) {    // Loop uma vez para cada entrada da EEPROM
    readID(i);          // Leia uma ID da EEPROM, ela é armazenada no armazenacartao[4]
    if ( checkTwo( find, armazenacartao ) ) {   // verifique se o cartão armazenado leu a partir da EEPROM
      return true;
    }
    else {    // Se não, retorna FALSO
    }
  }
  return false;
}

///////////////////////////////////////// Sucesso na gravação na EEPROM  ///////////////////////////////////
// Pisca o LED verde 3 vezes para indicar uma gravação bem-sucedida na EEPROM
void successWrite() {
  digitalWrite(led_amarelo, LED_OFF);   // Certifique-se de que o LED amarelo está apagado
  digitalWrite(led_vermelho, LED_OFF);  // Certifique-se de que o LED vermelho está apagado
  digitalWrite(led_verde, LED_OFF);  // Certifique-se de que o LED verde esteja aceso
  delay(200);
  digitalWrite(led_verde, LED_ON);   // Certifique-se de que o LED verde esteja aceso
  delay(200);
  digitalWrite(led_verde, LED_OFF);  // Certifique-se de que o LED verde está apagado
  delay(200);
  digitalWrite(led_verde, LED_ON);   // Certifique-se de que o LED verde esteja aceso
  delay(200);
  digitalWrite(led_verde, LED_OFF);  // Certifique-se de que o LED verde está apagado
  delay(200);
  digitalWrite(led_verde, LED_ON);   // Certifique-se de que o LED verde esteja aceso
  delay(200);
}

///////////////////////////////////////// Falha na gravação na EEPROM  ///////////////////////////////////
//Pisca o LED vermelho 3 vezes para indicar uma falha na gravação na EEPROM
void failedWrite() {
  digitalWrite(led_amarelo, LED_OFF);   // Certifique-se de que o LED amarelo está apagado
  digitalWrite(led_vermelho, LED_OFF);  // Certifique-se de que o LED vermelho está apagado
  digitalWrite(led_verde, LED_OFF);  // Certifique-se de que o LED verde está apagado
  delay(200);
  digitalWrite(led_vermelho, LED_ON);   // Certifique-se de que o LED vermelho está aceso
  delay(200);
  digitalWrite(led_vermelho, LED_OFF);  // Certifique-se de que o LED vermelho está apagado
  delay(200);
  digitalWrite(led_vermelho, LED_ON);   // Certifique-se de que o LED vermelho está aceso
  delay(200);
  digitalWrite(led_vermelho, LED_OFF);  // Certifique-se de que o LED vermelho está apagado
  delay(200);
  digitalWrite(led_vermelho, LED_ON);   // Certifique-se de que o LED vermelho está aceso
  delay(200);
}

///////////////////////////////////////// Sucesso para remover o ID da EEPROM ///////////////////////////////////
// Pisca o LED Amarelo 3 vezes para indicar o sucesso em excluir o ID da EEPROM
void successDelete() {
  digitalWrite(led_amarelo, LED_OFF);   // Certifique-se de que o LED amarelo está apagado
  digitalWrite(led_vermelho, LED_OFF);    // Certifique-se de que o LED vermelho está apagado
  digitalWrite(led_verde, LED_OFF);  //Certifique-se de que o LED verde esteja apagado
  delay(200);
  digitalWrite(led_amarelo, LED_ON);  // Certifique-se de que o LED amarelo esteja aceso
  delay(200);
  digitalWrite(led_amarelo, LED_OFF);   // Certifique-se de que o LED amarelo está apagado
  delay(200);
  digitalWrite(led_amarelo, LED_ON);  // Certifique-se de que o LED amarelo esteja aceso
  delay(200);
  digitalWrite(led_amarelo, LED_OFF);   // Certifique-se de que o LED amarelo está apagado
  delay(200);
  digitalWrite(led_amarelo, LED_ON);  //Certifique-se de que o LED amarelo esteja aceso
  delay(200);
}

////////////////////// Verificar se o cartão de leitura é o Cartão Mestre ///////////////////////////////////
// Verifique se o ID passado é o Cartão Mestre de programação
bool isMaster( byte test[] ) {
  return checkTwo(test, cartaomestre);
}

bool monitorbotaoutton(uint32_t interval) {
  uint32_t now = (uint32_t)millis();
  while ((uint32_t)millis() - now < interval)  {
    // check on every half a second
    if (((uint32_t)millis() % 500) == 0) {
      if (digitalRead(botao) != LOW)
        return false;
    }
  }
  return true;
}
