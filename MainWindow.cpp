/*
 * MIT License
 *
 * Copyright (c) 2021 Dan Logan
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "MainWindow.hpp"
#include <QInputDialog>

#include <QMessageBox>
#include <QLocale>

#define maxMatchs 5

void MainWindow::setupWindow()
{
    m_ui->playerList->setModel(&m_playerList);
    connect(m_ui->addPlayerB, &QPushButton::clicked, this, &MainWindow::addPlayer);
    connect(m_ui->removePlayerB, &QPushButton::clicked, this, &MainWindow::removePlayer);
    connect(m_ui->editPlayerNameB, &QPushButton::clicked, this, &MainWindow::editPlayerName);

    connect(m_ui->match1B, &QPushButton::clicked, std::bind(&MainWindow::generateMatch, this, 0));
    connect(m_ui->match2B, &QPushButton::clicked, std::bind(&MainWindow::generateMatch, this, 1));
    connect(m_ui->match3B, &QPushButton::clicked, std::bind(&MainWindow::generateMatch, this, 2));
    connect(m_ui->match4B, &QPushButton::clicked, std::bind(&MainWindow::generateMatch, this, 3));
    connect(m_ui->match5B, &QPushButton::clicked, std::bind(&MainWindow::generateMatch, this, 4));

    connect(m_ui->calcTourneyResB, &QPushButton::clicked, this, &MainWindow::calcFinalResult);

    m_ui->calcTourneyResB->setEnabled(false);
    m_ui->calcTourneyResB->setVisible(false);

    //create 5 matchs
    m_matches.emplace_back(m_ui->match1B, m_ui->match1T);
    m_matches.emplace_back(m_ui->match2B, m_ui->match2T);
    m_matches.emplace_back(m_ui->match3B, m_ui->match3T);
    m_matches.emplace_back(m_ui->match4B, m_ui->match4T);
    m_matches.emplace_back(m_ui->match5B, m_ui->match5T);
}

void MainWindow::addPlayer()
{
    bool ok;
    QString text = QInputDialog::getText(this, tr("Add Player"), tr("Player Name"), QLineEdit::Normal, "", &ok);
    if (ok && !text.isEmpty())
    {
        int32_t index = 0;
        for (const auto &player : m_players)
        {
            if (player->getId() >= index)
                index = player->getId() + 1;
        }
        m_players.emplace_back(std::make_shared<Player>(text, index));

        //update list
        updatePlayerList();
    }
}

void MainWindow::removePlayer()
{

    QModelIndexList selected = m_ui->playerList->selectionModel()->selectedIndexes();

    foreach (QModelIndex index, selected)
    {
        if (index.row() >= 0)
        { //QMap always sorts by key
            m_players.removeAt(index.row());
        }
    }

    updatePlayerList();
}

void MainWindow::editPlayerName()
{

    QModelIndexList selected = m_ui->playerList->selectionModel()->selectedIndexes();

    QList<int> rows;
    rows.clear();
    foreach (QModelIndex index, selected)
    {
        if (index.row() >= 0 && !rows.contains(index.row()))
        {
            //QMap always sorts by key
            rows.push_back(index.row());
        }
    }

    for (const auto index : rows)
    {
        if (m_players.size() <= index)
            continue;
        bool ok;
        QString text = QInputDialog::getText(this, tr("Edit Player"), tr("Player Name"), QLineEdit::Normal, m_players[index]->getName(), &ok);
        if (ok && !text.isEmpty())
        {
            m_players[index]->setName(text);
        }
    }

    updatePlayerList();
}

void MainWindow::updatePlayerList()
{
    QStringList players;
    for (const auto play : m_players)
    {
        players.push_back(play->getName());
    }
    m_playerList.setStringList(players);
    updatePlayerCount();
}

void MainWindow::updatePlayerCount()
{
    int matchCount = -1;
    int playerCount = m_players.size();
    if (playerCount == 0)
    {
        matchCount = 0;
        m_ui->calcTourneyResB->setVisible(false);
    }
    else if (playerCount <= 32)
    {
        matchCount = 3;
        if (playerCount > 8)
            matchCount++;
        if (playerCount > 16)
            matchCount++;
        m_ui->calcTourneyResB->setVisible(true);
    }

    for (int i = 0; i < maxMatchs; i++)
    {
        m_matches[i].setEnabled(i < matchCount);
    }
    m_matchCount = matchCount;
}

void MainWindow::generateMatch(int matchNum)
{
    bool genNext = true;
    if (matchNum > 0)
        genNext = m_matches[matchNum - 1].finalizeMatch(m_players, matchNum - 1);
    if (!genNext)
        return;
    m_matches[matchNum].generateMatch(m_players, matchNum);
    if ((matchNum + 1) >= m_matchCount)
        m_ui->calcTourneyResB->setEnabled(true);
}

void MainWindow::calcFinalResult()
{
    if (!m_matches[m_matchCount - 1].finalizeMatch(m_players, m_matchCount - 1))
    {
        return;
    }
    auto sortedList = m_players;

    std::sort(sortedList.begin(), sortedList.end(), [](std::shared_ptr<Player> p1, std::shared_ptr<Player> p2)
              { return p1->getTiebrokenScore() > p2->getTiebrokenScore(); });

    QString message;
    QTextStream messageBuilder(&message);
    std::int32_t place = 1;
    std::int32_t iterSize = 1;
    QLocale locale;
    for (std::int32_t i = 0; i < sortedList.size(); i++)
    {
        messageBuilder << locale.toString(place) << ": " << sortedList[i]->getName() << tr(", M:") << locale.toString(sortedList[i]->getMatchScore()) << tr(", G:") << locale.toString(sortedList[i]->getGameScore())
                       << tr(", MWP:") << locale.toString(sortedList[i]->getMatchWinPercentage(), 'f', 2) << tr(", GWP:") << locale.toString(sortedList[i]->getGameWinPercentage(), 'f', 2)
                       << tr(", OMWP:") << locale.toString(sortedList[i]->getOpponentMatchWinPercentage(), 'f', 2) << tr(", OGWP:") << locale.toString(sortedList[i]->getOpponentGameWinPercentage(), 'f', 2) << "\n";
        if (((i + 1) < sortedList.size()) && (sortedList[i]->getTiebrokenScore() == sortedList[i + 1]->getTiebrokenScore()))
        {
            iterSize++;
        }
        else
        {
            place += iterSize;
            iterSize = 1;
        }
    }

    QMessageBox dialog;
    dialog.setWindowTitle(tr("Tournament Results."));
    dialog.setText(message);
    dialog.exec();
}
