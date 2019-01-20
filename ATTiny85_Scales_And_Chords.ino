/*
 Name:		ATTiny85_Scales_And_Chords.ino
 Created:	1/6/2019 5:56:40 PM
 Author:	Alan
 Program size: 7,214 bytes (used 88% of a 8,192 byte maximum)
 Minimum Memory Usage: 268 bytes (52% of a 512 byte maximum)

*/

#include <SendOnlySoftwareSerial.h>
SendOnlySoftwareSerial midiSerial(0); // Tx PB 0 pin 5

const byte
lightSensitivity = 0, // the lower the more sensitive the light change detection;
arraySizeSet = 16,
//chordArraySizeSet = 8,
//keyArraySizeSet = 8,
channels = 16,
highHat = 42,
kick = 35,
snare = 38,
drumChan = 9,
bassChan = 0,
changeRatePin = 3,
tempoPin = 2, tunings[5] = { 40, 52, 64, 76, 88 },
LDRPin = 1;

const unsigned int scales[10] = {   // mirrored
	0x5AB5u, //Major
	0xD5ADu, //Minor
	0x94E9u, //Blues
	0xDB6Du, //Diminished
	0x5295u, //Maj Pentatonic
	0x5B55u, //Lydian Augmented
	0xDAADu, //Minor Melodic
	0x35B3u, //Gypsy
	0xD9ADu, //Harmonic Minor
	0xB5BBu //Flamenco
	//0xA54Au, //Black Keys
	//0x5555u  //Whole Tone
};
const unsigned int chords[16] = {  // mirrored
	0x0091u, //Major
	0x0891u, //Maj0r 7th
	0x1091u, //Major 9th
	0x0089u, //Minor
	0x0489u, //Minor 7th
	0x0289u, //Minor 6th
	0x148Du,  //Minor 9th
	0x00A1u, //Sus4
	0x04A1u, //7Sus4
	0x0111u, //Aug
	0x0491u, //7th
	0x0291u, //6th
	0x0049u, //Dim
	0x0001u, //Note
	0x0085u, //Sus2
	0x0485u  //7Sus2
};// , chord = 0x8520U;   

byte
filter1 = 0, flr1 = 0, fur1 = 70, filter1Rez = 80,
constChan = 5, constNote = 0,
drum = 0,
rul = 70, rll = 36, runNote = 60, crul = 40, crll = 36, crunNote = 40,
nextScale = 0,
constArraySize = arraySizeSet, bassArraySize = arraySizeSet, mel2ArraySize = arraySizeSet,
chordCount = 0, chordType = 0, arraySize = arraySizeSet,
bassrNote = 48, bassNote = 48, bul = 48, bll = 24,
melChan = 1, melNote = 60, mel2Chan = 3, mel2Note = 60,
pianoChan = 2, pianoChord = 0, pianoChordM = 0, pianoNote = 60, pianoNoteM = 60,
beat = 3, abeat = 1,
bassArray[arraySizeSet], mel2Array[arraySizeSet], tv[16], constArray[arraySizeSet];

unsigned long
bassPatt = 0, pianoPatt = 0, drumPatt = 0, synthPatt = 0, melPatt = 0, hhPatt = 0,
constPatt = 0, notePatt = 0, sc = 0, chordPatt = 0, mediumTime = 0, fastTime = 0, slowTime = 0;

unsigned int
//bassScale = 0, pianoScale = 0, tickScale = 0, synthScale = 0, windScale = 0,
playControl = 0, loopCount = 0, slowCount = 0, fastCount = 0, mediumCount = 0, straightCount = 0, scale = scales[0];

int
slowSpeed = 0, fastSpeed = 0, mediumSpeed = 0, lightLevel = 0, randomSpeed = 0, oldLightLevel = 0;

char
llc = 0, kn = 0, bassDir = 1, runDir = 1, crunDir = -1, filterDir1 = 1;

// the setup function runs once when you press reset or power the board
void setup() {
	pinMode(2, INPUT); //the slowSpeed speed pot - attiny85 PB2 pin 7
	pinMode(3, INPUT); //the note/lightLevel offset pot - attiny85 PB3 pin 2
	pinMode(1, OUTPUT);//output for the beat/activity LED - attiny85 PB1 pin 6
	pinMode(4, INPUT); //the LDR input for the lightLevel - attiny85 PB4 pin 3
	midiSerial.begin(31250); // Start serial port at the midi 31250 baud - out on attiny85 PB0 pin 5
	gsReset();  // reset the Boss DR-330 synth and switch to multitimberal mode
	delay(1000); //GS Reset needs a delay 


	for (byte c = 0; c < 16; c++) {
		ProgChange(c, wr());
		randomSeed(analogRead(LDRPin));
		pianoPatt = randomPatt(tr());
		chordPatt = randomPatt(tr());
		bassPatt = randomPatt(tr());
		melPatt = randomPatt(tr());
		beat = random(2, 4);
	}
	abeat = random(2, 4);
	ProgChange(drumChan, 0);
	ProgChange(bassChan, 32);
}

// the loop function runs over and over again until power down or reset
void loop() {
	oldLightLevel = lightLevel;
	lightLevel = analogRead(LDRPin);
	llc = oldLightLevel - lightLevel;

	if (millis() > fastTime) {
		slowSpeed = (unsigned long)(map(analogRead(tempoPin), 0, 1023, 4, 4960) + randomSpeed);  // 4/4 tempo control
		mediumSpeed = (unsigned long)((slowSpeed / beat));
		fastSpeed = (unsigned long)((mediumSpeed / abeat));//// tempo control fast beats and syncopation // hunamize

		if (bitRead(playControl, 0) && fastCount % (beat << 4) == 0) {
			drum = snare;
			NoteOn(drumChan, drum, 70);
			NoteOff(drumChan, drum);
		}
		else if (bitRead(playControl, 1) && fastCount % (abeat * 2) == 0) {
			drum = kick;
			NoteOn(drumChan, drum, 90);
			NoteOff(drumChan, drum);
		}
		else if (bitRead(playControl, 2) && fastCount % beat == 0) {
			drum = highHat + (random(3) * 2);
			NoteOn(drumChan, drum, 40);
			NoteOff(drumChan, drum);
		}
		if (bitRead(playControl, 3) && drumPatt >> fastCount % 32 & 1) {
			drum = kick + constArray[fastCount%arraySizeSet] % 36;
			NoteOn(drumChan, drum, 60);
			NoteOff(drumChan, drum);
		}



		if (bitRead(playControl, 4) && constPatt >> fastCount % 32 & 1) { //
			NoteOff(constChan, constNote);
			constNote = ScaleFilter(scale, kn + 24 + (
				constArray[slowCount%constArraySize] +
				mel2Array[mediumCount%mel2ArraySize] +
				bassArray[fastCount%bassArraySize]) % 72, kn);

			// filter work
			filter1 += filterDir1;
			if (filter1 >= fur1) {
				filterDir1 = -1;
				flr1 = rp();
			}
			if (filter1 <= flr1) {
				filterDir1 = 1;
				fur1 = hr();
			}
			DoFilter(constChan, filter1Rez, filter1);
			constArray[fastCount%constArraySize] -= llc;
			NoteOn(constChan, constNote, lr() + 40);   //********************** NOTE ON *************************

		}

		if (fastCount % 48 == 0) {
			NoteOff(constChan, constNote);
			constPatt = randomPatt(tr());
			constChan = randomChan();
			ProgChange(constChan, random(119));
			filter1Rez = wr();
			constArraySize = random(4, arraySizeSet + 1);
		}


		if (bitRead(playControl, 5) && pianoPatt >> fastCount % 32 & 1) {
			NoteOff(melChan, melNote);
			runNote += runDir;
			if (runNote >= rul) {
				rll = random(36, 48);
				runDir = -1;// 0 - (rp() + 1);
			}
			if (runNote <= rll) {
				rul = random(60, 80);
				runDir = 2;// rp() + 1;
			}
			melNote = ScaleFilter(scale, kn + runNote, kn); //********************** NOTE ON *************************
			NoteOn(melChan, melNote, hr());
		}

		if (bitRead(playControl, 6) && melPatt >> fastCount % mel2ArraySize & 1) {
			NoteOff(mel2Chan, mel2Note);
			mel2Note = ScaleFilter(scale, kn + 36 + (mel2Array[fastCount%mel2ArraySize]) % 60, kn);//ScaleFilter(scale, 36 + analogRead(LDRPin) % 60, kn); //
			mel2Array[fastCount%mel2ArraySize] += llc;
			NoteOn(mel2Chan, mel2Note, hr());  //********************** NOTE ON *************************
		}
		// Medium Time
		if (millis() > mediumTime) {
			tmv();
			rpan(randomChan(), lr());
			if (bitRead(playControl, 7) && mediumCount % (beat << 2) == 0) {
				drum = snare;
				NoteOn(drumChan, drum, 80);
				NoteOff(drumChan, drum);
			}
			else if (bitRead(playControl, 8) && mediumCount % (abeat * 2) == 0) {
				drum = kick;
				NoteOn(drumChan, drum, 100);
				NoteOff(drumChan, drum);
			}
			else {
				drum = highHat + (random(3) * 2);
				NoteOn(drumChan, drum, 50);
				NoteOff(drumChan, drum);
			}

			if (bitRead(playControl, 9) && bassPatt >> mediumCount % bassArraySize & 1) {
				NoteOff(bassChan, bassNote);
				/*bassrNote += bassDir;
				if (bassrNote >= bul) {
					bll = random(24, 36);
					bassDir = llc - 1;
				}
				if (bassrNote <= bll) {
					bul = random(36, 48);
					bassDir = llc + 1;
				}*/
				bassNote = ScaleFilter(scale, 24 + (bassArray[mediumCount%bassArraySize]) % 48, kn);//  bassrNote + ScaleFilter(scale, crunNote, kn);
				bassArray[mediumCount%arraySizeSet] += llc;
				NoteOn(bassChan, bassNote, hr());  //********************** NOTE ON *************************
			}

			if (mediumCount % 128 == 0) {
				NoteOff(mel2Chan, mel2Note);
				mel2Chan = randomChan();
				melPatt = randomPatt(tr());
				ProgChange(mel2Chan, random(119));
				abeat = random(2, 5);
			}

			if (mediumCount % 24 == 0) {
				pianoChord = getChord(scale);
				DoArticulations();
				drumPatt = randomPatt(tr());
				playControl |= 1 << (rp() % 16);
			}


			//Slow Time
			if (millis() > slowTime) {

				if (slowCount % (beat << 4) == 0) {
					drum = snare;
					NoteOn(drumChan, drum, 70);
					NoteOff(drumChan, drum);
				}
				else if (slowCount % (abeat * 2) == 0) {
					drum = kick;
					NoteOn(drumChan, drum, 90);
					NoteOff(drumChan, drum);
				}
				else if (slowCount % beat == 0) {
					drum = highHat + (random(3) * 2);
					NoteOn(drumChan, drum, 40);
					NoteOff(drumChan, drum);
				}

				analogWrite(1, 100);
				if (bitRead(playControl, 10) && chordPatt >> slowCount % 32 & 1) {
					playChord(chords[pianoChordM], pianoChan, pianoNoteM, 0, 0, chordType);
					chordType = tr();
					crunNote += crunDir;
					if (crunNote >= crul) {
						crll = random(24, 36);
						crunDir = 0 - (rp() - 1);

						////drum tunings
						CC(drumChan, 0x63, 0x18);
						CC(drumChan, 0x62, random(34, 60));
						CC(drumChan, 6, wr());

						//drum reverbs
						CC(drumChan, 0x63, 0x1D);
						CC(drumChan, 0x62, random(24, 60));
						CC(drumChan, 6, wr());

						//drum chorus
						CC(drumChan, 0x63, 0x1E);
						CC(drumChan, 0x62, random(24, 60));
						CC(drumChan, 6, wr());

					}
					if (crunNote <= crll) {
						crul = random(48, 60);
						crunDir = rp() + 1;
					}
					pianoNote = kn + crunNote;//  ScaleFilter(scale, crunNote, kn);
					pianoNoteM = pianoNote;
					pianoChordM = pianoChord;
					playChord(chords[pianoChordM], pianoChan, pianoNoteM, hr() - 30, 1, chordType);//********************** NOTE ON *************************
				}

				if (slowCount % 60 == 0) {
					bassArraySize = random(4, arraySizeSet + 1);
				}

				if (slowCount % random(40, 50) == 0) {
					mel2ArraySize = random(4, arraySizeSet + 1);
				}

				if (bitRead(playControl, 11) && slowCount % random(100) == 0) {
					ADSR(randomChan(), rp(), wr(), wr());
				}

				if (bitRead(playControl, 12) && slowCount % random(100) == 0) {
					MasterTune(randomChan(), tunings[random(5)]);
					ProgChange(drumChan, random(5) * 8);
				}


				if (slowCount % 48 == 0) {
					killPlayers();
					chordPatt = randomPatt(tr());
					scale = scales[random(10)];
					ProgChange(pianoChan, random(24));
					beat = random(2, 5);
					randomSpeed += random(11) - 5;
				}
				else if (slowCount % 24 == 0) {
					CC(randomChan(), 1, rp());  //slight modulation
					CC(randomChan(), 5, tr() << 1);  //portamento time
					CC(randomChan(), 65, wr());  //portamento on / off
					CC(randomChan(), 0x5B, wr()); // reverb send
					CC(randomChan(), 0x5D, wr()); // chorus send
					//killPlayers();
					bassPatt = randomPatt(tr());
					ProgChange(bassChan, random(31, 40));
				}
				else if (slowCount % 16 == 0) {
					killPlayers();
					playControl ^= 1 << (rp() % 16);
					kn = (lightLevel % 13) - 6;
					pianoPatt = randomPatt(tr());
					ProgChange(melChan, random(119));
				}

				analogWrite(1, 0);  // turn off the light change LED
				slowCount++;
				slowTime = (unsigned long)(millis() + (slowSpeed));
			} // end of the slow parts
			mediumCount++;
			mediumTime = (unsigned long)(millis() + (mediumSpeed));
		}// end of medium speed parts
		straightCount++;
		fastCount++;
		fastTime = (unsigned long)(millis() + (fastSpeed)); //  the other fast Time
	} // end of the fast parts
} // end of loop


//**********************************MACROS AND HELPERS********************************

// specificaly NRPN for BOSS DR - Synth 330
void MasterTune(byte chan, byte b) {
	//chan &= 0x0F;
	CC(chan, 0x65, 0);
	CC(chan, 0x64, 2);
	CC(chan, 6, b);
}
//specificaly NRPN for BOSS DR-Synth 330
void ADSR(byte chan, byte a, byte d, byte r) {
	chan &= 0x0F;
	////Attack
	CC(chan, 0x63, 0x01);
	CC(chan, 0x62, 0x63);
	CC(chan, 6, a);
	//decay
	CC(chan, 0x63, 0x01);
	CC(chan, 0x62, 0x64);
	CC(chan, 6, d);
	//release
	CC(chan, 0x63, 0x01);
	CC(chan, 0x62, 0x66);
	CC(chan, 6, r);
}

void rpan(byte chan, byte scope) {  // random pan 
	CC(chan, 10, random(64 - scope, 64 + scope));
}


byte randomChan() {
	byte rc = rp() % channels;
	while (rc == drumChan) { // rc < 4 || 
		rc = rp() % channels;
	}
	return rc;
}

void tmv() {
	//track mix volumes
	byte rcc = mediumCount % 16;
	(tv[rcc] >= 40 && tv[rcc] <= 70) ? (tv[rcc] += (tr() - 1)) : (tv[rcc] = 65);//
	CC(rcc, 7, rcc == 9 ? tv[rcc] - 10 : tv[rcc]);  // keep drum volumes down 10 behind rest of instruments
}

void killPlayers() {
	for (byte n = 0; n < 16; n++) {
		CC(n, 123, 0);
	}
}

byte getChord(unsigned int s) {
	byte ch = random(16);
	//match chord with scale in the key of c
	while ((s & chords[ch]) != chords[ch]) {
		ch = random(16);
	}
	return ch;
}


//chord plays the scale corrected chord
void playChord(unsigned int cord, byte chan, byte note, byte vel, byte cont, byte type) {
	// cont is either play or kill
	for (byte c = 0; c < 15; c++) {
		if (bitRead(cord & scale, c)) { // 
			if (cont) {
				NoteOn((type ? chan : (chan + c) % 16), note + c, vel); //kn + ScaleFilter(scale, note + c, kn)
			}
			else {
				NoteOff((type ? chan : (chan + c) % 16), note + c); // kn + ScaleFilter(scale, note + c, kn)
			}
		}
	}
}

// scale filtering - passed if the note belongs to the current scale, else make it a current chord note
byte ScaleFilter(unsigned int s, byte n, char k) {
	if (bitRead(s, ((n % 12 + k) % 12))) {
		return n;// +k;
	}
	else {  // add chord interval notes 
		while (pianoNoteM % 12 != n % 12) {  //align the root note
			n--;
		}
		unsigned int sc = chords[pianoChordM];
		byte cn = pianoNoteM % 16;       //note start point
		while (bitRead(sc, cn) == 0) {  //move until next highest chord note hit
			cn--;
		}
		n += cn;						// add that note onto root (could be 0 if it gets to root)
		return n;
	}
}


unsigned long randomPatt(byte r) { // random long patterns formed from nibbles, bytes or ints :)
	unsigned long n = random(1, 0xFFFFU);
	unsigned int y = random(1, 0xFFFFU);
	if (r == 0) {
		n = (((n & 0x0F) << 28) + ((n & 0x0F) << 24) + ((n & 0x0F) << 20) +
			((n & 0x0F) << 16) + ((n & 0x0F) << 12) + ((n & 0x0F) << 8) +
			((n & 0x0F) << 4) + (n & 0x0F));
	}
	else if (r == 1) {
		n = (((n & 0xFF) << 24) + ((n & 0xFF) << 16) + ((n & 0xFF) << 8) + (n & 0xFF));
	}
	else {
		n = ((n << 16) + y);
	}
	return n;
}


byte tr() {
	return random(3);
}


byte wr() {  // wide range random values for asdr and filter settings
	return random(0x0e, 0x72);
}

byte hr() {  // random high values for settings
	return random(65, 127);
}

byte lr() {  // random low values for settings
	return random(0, 64);
}

byte rp() {  // random bit pointers
	return random(32);
}

// send note on stuff
void NoteOn(byte chan, byte note, byte vel) {
	if (note > 23) {
		midiSerial.write(chan + 0x90);
		midiSerial.write(note & 0x7F);
		midiSerial.write(vel & 0x7F);
	}
}
// send note off stuff
void NoteOff(byte chan, byte note) {
	if (note > 23) {
		midiSerial.write(chan + 0x80);
		midiSerial.write(note & 0x7F);
		midiSerial.write(byte(0));
	}
}



//Basic Channel contro message
void CC(byte chan, byte cont, byte val) {
	//chan &= 0x0F;
	midiSerial.write((chan + 0xB0));
	midiSerial.write(cont & 0x7F);
	midiSerial.write(val & 0x7F);
}

//specificaly NRPN for BOSS DR-Synth 330
void DoFilter(byte ch, byte res, byte coff) {
	//ch &= 0x0F;
	CC(ch, 0x63, 0x01);
	CC(ch, 0x62, 0x21);
	CC(ch, 6, res & 0x7F); //resonance can go to 0x72
	CC(ch, 0x63, 0x01);
	CC(ch, 0x62, 0x20);
	CC(ch, 6, coff & 0x7F);//cut off frequency
}

void DoArticulations() {  // for Kontact
	byte articnote = random(23, 127);
	byte rcc = random(16);
	NoteOn(rcc, articnote, 1);
	NoteOff(rcc, articnote);
}

//specificaly sysex for BOSS DR-Synth DS330
void gsReset() {
	byte gs[11] = { 0xF0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x7F, 0x00, 0x41, 0xF7 };
	for (byte g = 0; g < 11; g++) {
		midiSerial.write(gs[g]);
	}
}

// Program change for midi channel
void ProgChange(byte chan, byte prog) {
	//chan &= 0x0F;
	midiSerial.write((chan + 0xC0));
	midiSerial.write(prog & 0x7F);
}