# Statistical POS Tagger for Slovak Language

This is a POS tagging tool for Slovak language, which is based on CRFsuite and uses our pre-trained model.

## Requirements and Setup

This program is dependent on:  
[CRFsuite](http://www.chokkan.org/software/crfsuite/) made by Naoki Okazaki  
[The Stanford Parser](http://nlp.stanford.edu/software/lex-parser.shtml) (Java application)  
  
It is required to download both programs.  
Then add CRFsuite folder to PATH environment variable or ensure, that the program has access to *crfsuite.exe*  
Also add *stanford-parser.jar* from stanford parser folder to CLASSPATH, or ensure that Java has full access to classes of this file.  
  
To use the program you need one of these pre-trained models. The second one uses additional word vectors, which encode various latent features of words. The word vectors were trained with [word2vec](https://code.google.com/p/word2vec/).

1. [crf-10pct.mdl](https://drive.google.com/file/d/0B54yBSwttuiDeWtaa3BGRnJiLVk/view?usp=sharing)  
2. [crf-vec-1pct.mdl](https://drive.google.com/file/d/0B54yBSwttuiDck54OUdadFhYdWs/view?usp=sharing)

Word vectors:
[vec-300sk.bin](https://drive.google.com/file/d/0B54yBSwttuiDQmJPTGFRQ2FhNXc/view?usp=sharing)  

It is recommended to put these files to the same folder as the program is located.

The program runs in command line and has a self explaining --help switch command.
To compile the program in slovak language uncomment
main.cpp:5 #define LANGUAGE_SLOVAK  
  
## Acknowledgement
  
The model was trained on [Slovak National Corpus dataset](http://korpus.juls.savba.sk/wiki.html).
  
-------------------------------------------------------------------------------------------------------------  
# Štatistický značkovač slovných druhov pre slovenčinu

Predstavujeme Vám nástroj pre určovanie slovných druhov v slovenčine, ktorý je postavený na CRFsuite a využíva náš natrénovaný model.

## Požiadavky a inštalácia

Tento program je závislý na:
[CRFsuite](http://www.chokkan.org/software/crfsuite/), autor Naoki Okazaki  
[The Stanford Parser](http://nlp.stanford.edu/software/lex-parser.shtml) (Java aplikácia)  
  
Je potrebné stiahnuť oba programy.  
Následne je nutné pridať priečinok CRFsuite do premenných prostredia PATH, alebo zabezpečiť, aby mal program prístup k *crfsuite.exe*  
Tiež je nutné pridať *stanford-parser.jar* do premennej CLASSPATH, alebo inak zabezpečiť prístup k triedam tejto aplikácie.

Pre použitie značkovača je nutné stiahnuť jeden z nasledovných natrénovaných modelov. Druhý z nich využíva prídavné vektory slov, ktoré kódujú rôzne latentné črty slov. Vektory slov boli natrénované s nástrojom [word2vec](https://code.google.com/p/word2vec/).

1. [crf-10pct.mdl](https://drive.google.com/file/d/0B54yBSwttuiDeWtaa3BGRnJiLVk/view?usp=sharing)  
2. [crf-vec-1pct.mdl](https://drive.google.com/file/d/0B54yBSwttuiDck54OUdadFhYdWs/view?usp=sharing)

Vektory slov:
[vec-300sk.bin](https://drive.google.com/file/d/0B54yBSwttuiDQmJPTGFRQ2FhNXc/view?usp=sharing)
 
Odporúča sa vložiť tieto súbory do rovnakého priečinka, v akom sa nachádza program.

Tento program sa spúšťa z konzoly a obsahuje prepínač --help, ktorý vypíše použitie programu.
Ak chcete skompilovať program v slovenskom jazyku, odkomentujte  
main.cpp:5 #define LANGUAGE_SLOVAK  
  
## Poďakovanie
  
Náš model bol natrénovaný na datasete [Slovenského národného korpusu](http://korpus.juls.savba.sk/wiki.html).
