/*
  Copyright (c) 2012, BuildmLearn Contributors listed at http://buildmlearn.org/people/
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

  * Neither the name of the BuildmLearn nor the names of its
    contributors may be used to endorse or promote products derived from
    this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "templates/learnspellings/learnspellingssimulator.h"

#include "core/templatecore.h"
#include "gui/custommessagebox.h"
#include "network-web/networkfactory.h"
#include "miscellaneous/iofactory.h"
#include "templates/learnspellings/learnspellingseditor.h"

#include <QInputDialog>
#include <QFile>
#include <QTextStream>
#include <QDataStream>


LearnSpellingsSimulator::LearnSpellingsSimulator(TemplateCore *core, QWidget *parent)
  : TemplateSimulator(core, parent), m_ui(new Ui::LearnSpellingsSimulator), m_words(QList<LearnSpellingsItem>()),
    m_activeWord(-1) {
  m_ui->setupUi(this);
  m_ui->m_phoneWidget->setStyleSheet("background: #255593; color: white;");

  connect(m_ui->m_btnPlayWord, SIGNAL(clicked()), this, SLOT(playWord()));
  connect(m_ui->m_btnStart, SIGNAL(clicked()), this, SLOT(start()));
  connect(m_ui->m_btnRestart, SIGNAL(clicked()), this, SLOT(restart()));
  connect(m_ui->m_btnExit, SIGNAL(clicked()), this, SLOT(exit()));
  connect(m_ui->m_btnSkip, SIGNAL(clicked()), this, SLOT(skipThisWord()));
  connect(m_ui->m_btnSpellIt, SIGNAL(clicked()), this, SLOT(spellThisWord()));
  connect(m_ui->m_btnGoToNextWord, SIGNAL(clicked()), this, SLOT(loadNextWord()));
}

LearnSpellingsSimulator::~LearnSpellingsSimulator() {
  delete m_ui;
}

bool LearnSpellingsSimulator::startSimulation() {
  LearnSpellingsEditor *editor = static_cast<LearnSpellingsEditor*>(core()->editor());

  if (!editor->canGenerateApplications()) {
    // There are no active questions or quiz does not
    // containt its name or author name.
    return false;
  }

  // Load the questions, setup the quiz and start it.
  m_words = editor->activeWords();

  if (m_words.isEmpty()) {
    return false;
  }

  m_ui->m_btnStart->setEnabled(true);
  m_ui->m_lblAuthor->setText(editor->m_ui->m_txtAuthor->lineEdit()->text());
  m_ui->m_lblHeading->setText(editor->m_ui->m_txtName->lineEdit()->text());
  m_ui->m_phoneWidget->setCurrentIndex(1);

  return true;
}

bool LearnSpellingsSimulator::stopSimulation() {
  m_ui->m_phoneWidget->setCurrentIndex(0);

  emit canGoBackChanged(false);

  return true;
}

bool LearnSpellingsSimulator::goBack() {
  return false;
}

void LearnSpellingsSimulator::start() {
  m_activeWord = -1;
  m_resultCorrect = m_resultIncorrect = m_resultSkipped = 0;
  loadNextWord();
}

void LearnSpellingsSimulator::restart() {
  m_ui->m_phoneWidget->setCurrentIndex(1);
}

void LearnSpellingsSimulator::exit() {
  stopSimulation();
  emit simulationStopRequested();
}

void LearnSpellingsSimulator::playWord() {
  // TODO: Play sound.
  QByteArray output;

  QString word = m_words.at(m_activeWord).word().replace(' ', '+');
  QString url = QString("http://mary.dfki.de:59125/process?INPUT_TEXT=%1&INPUT_TYPE=TEXT&OUTPUT_TYPE=AUDIO&AUDIO=WAVE_FILE&LOCALE=en_US").arg(word);

  QNetworkReply::NetworkError err = NetworkFactory::downloadFile(url, 10000, output);

  // TODO: Pokračovat.
  QFile ff("M:\\aaa.wav");
  ff.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Unbuffered);
  ff.write(output);
  ff.close();

  IOFactory::playWaveFile("M:\\aaa.wav");

  m_ui->m_btnSkip->setEnabled(true);
  m_ui->m_btnSpellIt->setEnabled(true);
}

void LearnSpellingsSimulator::skipThisWord() {
  m_resultSkipped++;
  loadNextWord();
}

void LearnSpellingsSimulator::spellThisWord() {
  // TODO: display input box for spell word, if answer is correct
  // then increment.
  // then display "word result page"
  QString guessed_word = QInputDialog::getText(this, tr("Enter spelling"), tr("Enter spelling")).simplified();
  LearnSpellingsItem current_word = m_words.at(m_activeWord);

  if (!guessed_word.isEmpty()) {
    if (guessed_word == current_word.word()) {
      // User guessed the word!!!
      m_resultCorrect++;
      m_ui->m_lblResultCaption->setText(tr("This is correct spelling"));
    }
    else {
      // User made a mistake.
      m_resultIncorrect++;
      m_ui->m_lblResultCaption->setText(tr("This is not the correct spelling"));
    }

    // Display page with overview of current word status.
    m_ui->m_lblEnteredWord->setText(tr("You entered %1").arg(guessed_word));
    m_ui->m_lblCorrectWordDescription->setText(tr("<p style=\" font-size: 18pt;\">%1</p><p>%2</p>").arg(current_word.word(), current_word.meaning()));
    m_ui->m_listener->setCurrentIndex(1);
  }
  else {
    CustomMessageBox::show(this, QMessageBox::Warning, tr("Enter some word"), tr("You must enter some word"));
  }
}

void LearnSpellingsSimulator::loadNextWord() {
  m_activeWord++;
  m_ui->m_lblQuestionNumber->setText(tr("Word #%1 of %2").arg(QString::number(m_activeWord + 1),
                                                              QString::number(m_words.size())));

  if (m_activeWord < m_words.size()) {
    // We are not viewing last word, display the "listening" page.
    m_ui->m_btnSkip->setEnabled(false);
    m_ui->m_btnSpellIt->setEnabled(false);
    m_ui->m_listener->setCurrentIndex(0);
    m_ui->m_phoneWidget->setCurrentIndex(2);
  }
  else {
    // We are already viewing last word, we need to go to "summary" page and display results.
    m_ui->m_lblTotalCorrect->setText(tr("Correct spelled %1").arg(QString::number(m_resultCorrect)));
    m_ui->m_lblTotalUnanswered->setText(tr("Unanswered %1").arg(QString::number(m_resultSkipped)));
    m_ui->m_lblTotalWrong->setText(tr("Wrond spelled %1").arg(QString::number(m_resultIncorrect)));
    m_ui->m_phoneWidget->setCurrentIndex(3);
  }
}
