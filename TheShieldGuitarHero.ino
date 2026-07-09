#include <TimerOne.h>
#include <MultiFuncShield.h>
#include <SoftwareSerial.h>

// ===============================
// BLUETOOTH
// ===============================
SoftwareSerial BT(10, 11); // RX, TX

// ===============================
// VARIÁVEIS DO JOGO
// ===============================
int ledAtual = 0;
int vidas = 3;
int score = 0;

bool jogoIniciado = false;
bool jogoFinalizado = false;
bool ledAtivo = false;

int tempoInicial = 30;
int tempoRestante = 30;
int ultimoTempoExibido = -1; // Evita atualizar o display sem necessidade

unsigned long ultimoSegundo = 0;
unsigned long tempoLedLigado = 0;
unsigned long tempoEsperaStart = 0;
bool aguardandoStart = false;

int tempoResposta = 1000; 

// ===============================
// ENVIA DADOS PARA BLUETOOTH
// ===============================
void enviarBluetooth() {
  BT.print("SCORE:");
  BT.print(score);
  BT.print(" | VIDAS:");
  BT.print(vidas);
  BT.print(" | TEMPO:");
  BT.println(tempoRestante);
}

// ===============================
// GERA LED ALEATÓRIO
// ===============================
void gerarNovoLed() {
  ledAtual = random(1, 5);

  MFS.writeLeds(LED_ALL, OFF);

  switch(ledAtual) {
    case 1: MFS.writeLeds(LED_1, ON); break;
    case 2: MFS.writeLeds(LED_2, ON); break;
    case 3: MFS.writeLeds(LED_3, ON); break;
    case 4: MFS.writeLeds(LED_4, ON); break;
  }

  tempoLedLigado = millis();
  ledAtivo = true;
}

// ===============================
// GAME OVER (DERROTA)
// ===============================
void tocarDerrota() {
  MFS.write("FAIL"); 
  BT.println("\n=== GAME OVER ===");
  BT.print("Pontuacao Final: ");
  BT.println(score);

  while(true) {
    MFS.writeLeds(LED_ALL, ON);
    MFS.beep(10, 10, 1);
    delay(200); // Aqui pode delay porque o jogo acabou e travou mesmo
    MFS.writeLeds(LED_ALL, OFF);
    delay(200);
  }
}

// ===============================
// VITÓRIA
// ===============================
void tocarVitoria() {
  MFS.write("WIN ");
  BT.println("\n=== VOCE VENCEU! ===");
  BT.print("Pontuacao Final: ");
  BT.println(score);

  for(int i = 0; i < 5; i++) {
    MFS.beep(50, 50, 1);
    MFS.writeLeds(LED_ALL, ON);
    delay(100);
    MFS.writeLeds(LED_ALL, OFF);
    delay(100);
  }

  while(true);
}

// ===============================
// VERIFICA ENTRADA DO JOGADOR
// ===============================
void verificarEntrada(int botaoPressionado) {
  if(botaoPressionado == ledAtual || (ledAtual == 4 && botaoPressionado == 3)) {
    MFS.beep(5, 5, 1); 
    score++; 

    if(tempoResposta > 250) {
      tempoResposta -= 30;
    }

    enviarBluetooth();
    gerarNovoLed();
  } else {
    vidas--;
    BT.println("ERRO! -1 Vida");
    MFS.beep(50, 50, 2); 
    
    enviarBluetooth();

    if(vidas <= 0) {
      jogoFinalizado = true;
      tocarDerrota();
    } else {
      gerarNovoLed(); 
    }
  }
}

// ===============================
// TIMEOUT DO LED
// ===============================
void verificarTimeout() {
  if(ledAtivo && (millis() - tempoLedLigado >= tempoResposta)) {
    vidas--;
    BT.println("TIMEOUT! -1 Vida");
    MFS.beep(100, 50, 1);
    
    enviarBluetooth();

    if(vidas <= 0) {
      jogoFinalizado = true;
      tocarDerrota();
    } else {
      gerarNovoLed(); 
    }
  }
}

// ===============================
// SETUP
// ===============================
void setup() {
  Timer1.initialize();
  MFS.initialize();

  Serial.begin(9600);   
  BT.begin(9600);       

  randomSeed(analogRead(A1)); 
  BT.println("Guitar Hero MFS Pronto!");
}

// ===============================
// LOOP PRINCIPAL
// ===============================
void loop() {
  
  // ============================
  // TELA INICIAL (MENU)
  // ============================
  if(!jogoIniciado) {
    if (!aguardandoStart) {
      int valorPot = analogRead(A0); 
      tempoInicial = map(valorPot, 0, 1023, 10, 90);
      tempoRestante = tempoInicial;

      // Só atualiza o display se o valor do potenciômetro mudar! (Evita o bug visual)
      if (tempoInicial != ultimoTempoExibido) {
        MFS.write(tempoInicial);
        ultimoTempoExibido = tempoInicial;
      }

      byte botao = MFS.getButton();
      if(botao) {
        byte acao = botao & B11000000;
        if(acao == BUTTON_PRESSED_IND) {
          MFS.write("GO");
          BT.println("\n--- JOGO INICIADO ---");
          enviarBluetooth();
          MFS.beep(50, 50, 3);
          
          // Prepara a transição sem usar delay() trancando o chip
          tempoEsperaStart = millis();
          aguardandoStart = true;
        }
      }
    } else {
      // Espera 1 segundo de forma "limpa" para dar o START
      if (millis() - tempoEsperaStart >= 1000) {
        jogoIniciado = true;
        aguardandoStart = false;
        ultimoSegundo = millis();
        gerarNovoLed();
      }
    }
    return;
  }

  if(jogoFinalizado) return;

  // ============================
  // CRONÔMETRO DO JOGO
  // ============================
  if(millis() - ultimoSegundo >= 1000) {
    ultimoSegundo = millis();
    tempoRestante--;

    // Só manda pro display se o tempo mudou, impedindo conflito com a atualização dos LEDs
    MFS.write(tempoRestante); 
    enviarBluetooth();

    if(tempoRestante <= 0) {
      jogoFinalizado = true;
      tocarVitoria();
    }
  }

  // ============================
  // VERIFICA TIMEOUT DO LED
  // ============================
  verificarTimeout();

  // ============================
  // LEITURA DOS BOTÕES DO JOGADOR
  // ============================
  byte botao = MFS.getButton();
  if(botao) {
    byte numeroBotao = botao & B00111111; 
    byte acao = botao & B11000000;

    if(acao == BUTTON_PRESSED_IND) {
      verificarEntrada(numeroBotao);
    }
  }
}