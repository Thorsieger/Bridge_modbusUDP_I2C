#include <Wire.h>
#include <NanoESP.h>

#define SSID ""//ssid
#define password "" //password 
unsigned port = 502;//port modbus

#define LED_Ecriture 15 //LED rouge
#define LED_Lecture 17 //LED verte

NanoESP nanoesp = NanoESP();


unsigned I2C[3]={};//Tableau des adresses I2C
//byte msg[20];//trame recu par wifi

void setup() {
//Setup LED
  pinMode(LED_Ecriture,OUTPUT);
  pinMode(LED_Lecture,OUTPUT);
  
//Connexion série
  Serial.begin(9600);
  
//Connexion Wifi/UDP
  connexionWifi();

//Connaitre les modules connecté au réseau I2C
  connexionI2C();

  Serial.println("Fin de l'initialisation");
  //Les deux LEDs s'allument 400ms pour un débogage visuel de fin de setup
  digitalWrite(LED_Ecriture, HIGH);digitalWrite(LED_Lecture, HIGH);
  delay(400);
  digitalWrite(LED_Ecriture, LOW);digitalWrite(LED_Lecture, LOW);
}

void loop() {
  int client,len;
  if(nanoesp.recvData(client,len))//Reception de données
  {
    Serial.println("messageRecu!");
    byte msg[len+1];//taille+1 à cause de ":" recu
    Serial.print("Taille Trame :");
    Serial.println(len);
    nanoesp.readBytes(msg,len+1);//reception de la trame openmodbus
    Serial.print("12eme valeur :");
     Serial.println(msg[12]);
     for(int i=0; i<len;i++)//afficahge de la trame recu
     {
      Serial.print(msg[i]);
     }
    if(sizeof(msg)==(int)msg[6]+7){//test taille trame
    if(msg[8]==1 || msg[8]==2 ||msg[8]==3 ||msg[8]==4 )//code fonction lecture
    {
      Serial.println("lecture");
      lectureI2C(client,msg);
    }
    else if(msg[8]==5||msg[8]==6||msg[8]==15||msg[8]==16)//code fonction ecriture
    {
      Serial.println("ecriture");
      ecritureI2C(client,msg);
    }
    else//autre codes
    {
      Serial.println("Autre code fonction");
      byte rep[] = {msg[1] ,msg[2] ,msg[3] ,msg[4] ,msg[5] ,3 ,msg[7] ,(msg[8]+128),1};//réponse codé d'erreur fonction
      envoie(client,rep,sizeof(rep));
    }
  }
  else{
    Serial.println("erreur Taille");
    }
  // erreur taille trame donc pas de réponse
  }
}


void connexionWifi(){
  nanoesp.init();
   if(nanoesp.configWifi(STATION,SSID,password))//connexion au réseau wifi
  {
    Serial.println("Connexion Wifi OK\n"+ (String)nanoesp.getIp());//Affichage adresse IP
  }
  else
  {
    Serial.println("Connexion Wifi non OK\n");
     //debug
    digitalWrite(LED_Ecriture, HIGH); 
    while(true){//LED verte clignote
      digitalWrite(LED_Lecture, HIGH);
      delay(500);
      digitalWrite(LED_Lecture, LOW);
      delay(500);
    }
  }
  if(nanoesp.startUdpServer(0,"0.0.0.0",port,port,2))//Ouverture du port UDP
  {
    Serial.println("Connexion UDP OK");
  }
  else
  {
    Serial.println("Connexion UDP non OK\n");
    //debug
    digitalWrite(LED_Ecriture, HIGH); 
    while(true){//LED verte clignote
      digitalWrite(LED_Lecture, HIGH);
      delay(500);
      digitalWrite(LED_Lecture, LOW);
      delay(500);
    }
  }
}


void connexionI2C(){
  Wire.begin();
  int j=0;
  for(int test=0;test<127;test++)//test des adresses I2C
  {
    Wire.requestFrom(test, 1);//demande de 'test' un octet
    if(Wire.available()!=0)//Si on a une réponse
    {
      I2C[j]=test;//ajout au tableau d'adresse
      j++;
      Serial.println("L'esclave I2C " + String(j) + " a pour adresse : " + String(test) );
    }    
  }
  if(I2C[0]!=0)
  {
    Serial.println("Liste des adresses I2C cree");
  }
  else
  {
    Serial.println("Aucun module I2C detecte");
     //debug
    digitalWrite(LED_Lecture, HIGH); 
    while(true){//LED rouge clignote
      digitalWrite(LED_Ecriture, HIGH);
      delay(500);
      digitalWrite(LED_Ecriture, LOW);
      delay(500);
    }
  }
}

void lectureI2C(int client, byte msg[])
{
  Serial.println(I2C[msg[10]-1]);
  Serial.println(msg[10]);
  if(I2C[msg[10]-1]!=0)//l'adresse esclave existe
  { 
    Serial.println("okAdd");
    digitalWrite(LED_Lecture, HIGH);//allumage LED lecture (verte)
    Wire.requestFrom(I2C[msg[10]-1], 1);//Demande d'un octet à l'esclave I2C
    if(Wire.available()!=0)//eslcave I2C répond
    { 
      Serial.println("RepI2C");
       byte c = Wire.read();
       byte rep[] = {msg[1] ,msg[2] ,msg[3] ,msg[4] ,msg[5] ,4 ,msg[7] ,msg[8] ,1 ,c};//message renvoie valeur
      envoie(client,rep,sizeof(rep));
       Serial.println("repWifiSend");
    }
    else
    {
      byte rep[] = {msg[1] ,msg[2] ,msg[3] ,msg[4] ,msg[5] ,3 ,msg[7] ,(msg[8]+128),11};//trame d'erreur réponse I2C
   envoie(client,rep,sizeof(rep));
    Serial.println("pasRepI2C");
    }
    digitalWrite(LED_Lecture, LOW);//extinction de la LED
  }
  else
  {
    Serial.println("nokAdd");
    byte rep[] = {msg[1] ,msg[2] ,msg[3] ,msg[4] ,msg[5] ,3 ,msg[7] ,(msg[8]+128),10};//trame réponse addresse I2C inexistante
   envoie(client,rep,sizeof(rep));
  }
}

void ecritureI2C(int client,byte msg[])
{
  Serial.println(I2C[msg[10]-1]);
  Serial.println(msg[10]);
  if(I2C[msg[10]-1]!=0)//l'adresse esclave existe
  { 
    
    Serial.println("okAdd");
    if(msg[11]!=0)//vérification de la valeur a écrire
      {
        byte rep[] = {msg[1] ,msg[2] ,msg[3] ,msg[4] ,msg[5] ,4 ,msg[7] ,(msg[8]+128),3};//erreur valeur envoyé
       envoie(client,rep,sizeof(rep));
        Serial.println("Erreur Valeur");
      }
      else
      {
        digitalWrite(LED_Ecriture, HIGH);//allumage LED ecriture (rouge)
        int addresse = I2C[msg[10]-1];
        Wire.beginTransmission(addresse);//Connexion pour écriture

        //Valeur a envoyer : forcer les bits de gauche a 1 avec un ou : x=x|10101010 // Utilisation d'un masque
        int val = msg[12]|B10101010;
        Wire.write(val); // envoyer la valeur
        if(Wire.endTransmission()==0)//réponse I2C
        {
          Serial.println("RepI2C");
          byte rep[] = {msg[1] ,msg[2] ,msg[3] ,msg[4] ,msg[5] ,6 ,msg[7] ,msg[8],msg[9],msg[10],msg[11],msg[12]}; //réponse : meme trame que recu
         envoie(client,rep,sizeof(rep));
          Serial.println("repWifiSend");
        }
        else
        {
          Serial.println("pas2RepI2C");
          byte rep[] = {msg[1] ,msg[2] ,msg[3] ,msg[4] ,msg[5] ,3 ,msg[7] ,(msg[8]+128),11};//réponse pas de réponse I2C
         envoie(client,rep,sizeof(rep));
        }
        digitalWrite(LED_Ecriture, LOW);//extinction de la LED
      }
  }
  else
  {
    Serial.println("nokAdd");
    byte rep[] = {msg[1] ,msg[2] ,msg[3] ,msg[4] ,msg[5] ,4 ,msg[7] ,(msg[8]+128),10}; //réponse adresse I2C inexistante
   envoie(client,rep,sizeof(rep));
  }
}

void envoie(int client,byte rep[],int len)//Correspond à la fonction 'sendRaw()' mais qui etait très lente (ici c'est un simple copié/coller de la fonction)  
{//envoie le message de réponse
  nanoesp.sendCom("AT+CIPSEND=" + String(client) + "," + String(len), ">");
      for (size_t i = 0; i <= len; i++)
        nanoesp.write(rep[i]);
}
