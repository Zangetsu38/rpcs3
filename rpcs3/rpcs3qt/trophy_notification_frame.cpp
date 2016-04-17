#include "trophy_notification_frame.h"
#include "Emu/System.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>

static const int TROPHY_TIMEOUT_MS = 7500;

constexpr auto qstr = QString::fromStdString;

trophy_notification_frame::trophy_notification_frame(const std::vector<uchar>& imgBuffer, const SceNpTrophyDetails& trophy, int height) : QWidget()
{
	setObjectName("trophy_notification_frame");
	setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
	setAttribute(Qt::WA_ShowWithoutActivating);

	// Fill the background with black
	QPalette black_background;
	black_background.setColor(QPalette::Window, Qt::black);
	black_background.setColor(QPalette::WindowText, Qt::white);

	// Make the label
	QLabel* trophyImgLabel = new QLabel;
	trophyImgLabel->setAutoFillBackground(true);
	trophyImgLabel->setPalette(black_background);

	QImage trophyImg;
	if (!imgBuffer.empty() && trophyImg.loadFromData((uchar*)&imgBuffer[0], static_cast<int>(imgBuffer.size()), "PNG"))
	{
		trophyImg = trophyImg.scaledToHeight(height); // I might consider adding ability to change size since on hidpi this will be rather small.
		trophyImgLabel->setPixmap(QPixmap::fromImage(trophyImg));
	}
	else
	{
		// This looks hideous, but it's a good placeholder.
		trophyImgLabel->setPixmap(QPixmap::fromImage(QImage(":/rpcs3.ico")));
	}

	QLabel* trophyName = new QLabel;
	trophyName->setWordWrap(true);
	trophyName->setAlignment(Qt::AlignCenter);

	QLabel* trophyType = new QLabel;
	switch (trophy.trophyGrade)
	{
	case SCE_NP_TROPHY_GRADE_BRONZE: trophyType->setPixmap(QPixmap::fromImage(QImage(":/Icons/trophy_tex_grade_bronze.png"))); break;
	case SCE_NP_TROPHY_GRADE_SILVER: trophyType->setPixmap(QPixmap::fromImage(QImage(":/Icons/trophy_tex_grade_silver.png"))); break;
	case SCE_NP_TROPHY_GRADE_GOLD: trophyType->setPixmap(QPixmap::fromImage(QImage(":/Icons/trophy_tex_grade_gold.png"))); break;
	case SCE_NP_TROPHY_GRADE_PLATINUM: trophyType->setPixmap(QPixmap::fromImage(QImage(":/Icons/trophy_tex_grade_platinum.png"))); break;
	default: break;
	}

	QString TrophyMsg = "";
	switch (g_cfg.sys.language)
	{
	case 0: TrophyMsg = "トロフィーを獲得しました。"; break;//Japanese
	case 1: TrophyMsg = "You have earned a trophy."; break; //English(US)
	case 2: TrophyMsg = "Vous avez obtenue un trophée."; break; //French
	case 3: TrophyMsg = "Has ganado un trofeo."; break; //Spanish
	case 4: TrophyMsg = "Sie haben eine Trophäe verdient."; break; //German
	case 5: TrophyMsg = "Hai guadagnata un trofeo"; break; //Italian
	//case 6: TrophyMsg = "Du hast eine Trophäe erhalten."; break; //Dutch
	case 6: TrophyMsg = "U hebt een trofee gewonnen."; break; //Dutch	
	case 7: TrophyMsg = "Ganhaste um troféu"; break; //Portuguese (PT)
	case 8: TrophyMsg = "Приз получен."; break;//Russian
	case 9: TrophyMsg = "트로피를 획득했습니다."; break; //Korean
	case 10: TrophyMsg = "已獲得獎盃."; break; //Chinese (Trad.)
	//case 11: TrophyMsg = ""; break; //Chinese (Simp.)
	//case 12: TrophyMsg = ""; break; //Finnish
	case 13: TrophyMsg = "Du har vunnit en trophy."; break; //Swedish
	//case 14: TrophyMsg = ""; break; //Danish
	case 15: TrophyMsg = "Udalo Ci sie zdobyc trofeum."; break; //Polish
	case 16: TrophyMsg = "Trophy earned."; break; //English (UK)
	case 17: TrophyMsg = "Você conquistou um troféu."; break; //Portuguese (BR)
	case 18: TrophyMsg = "Kupa kazanildi."; break; //Turkish
	}

	trophyName->setText(TrophyMsg + "\n" + qstr(trophy.name));
	trophyName->setAutoFillBackground(true);
	trophyName->setPalette(black_background);

	QHBoxLayout* globalLayout = new QHBoxLayout;
	globalLayout->addWidget(trophyImgLabel);
	globalLayout->addWidget(trophyType);
	globalLayout->addWidget(trophyName);
	setLayout(globalLayout);
	setPalette(black_background);

	// I may consider moving this code later to be done at a better location.
	QTimer::singleShot(TROPHY_TIMEOUT_MS, [this]()
	{
		deleteLater();
	});
}
