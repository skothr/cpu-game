#include "worldCreate.hpp"
#include "button.hpp"

#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QString>
#include <QPushButton>
#include <QComboBox>

WorldCreate::WorldCreate(QWidget *parent)
  : QWidget(parent), mOptions{"new-world", terrain_t::PERLIN_WORLD, 0, {0,0,0}, {8,8,4}, 8, 8}
{
  QLabel *nLabel = new QLabel("World Name: ");
  QLineEdit *nameText = new QLineEdit();
  nameText->setText(mOptions.name.c_str());
  connect(nameText, SIGNAL(textChanged(const QString&)), this, SLOT(setName(const QString&)));
  QHBoxLayout *name = new QHBoxLayout();
  name->addWidget(nLabel);
  name->addWidget(nameText);
  QWidget *nameWidget = new QWidget(this);
  nameWidget->setLayout(name);
  
  QLabel *tLabel = new QLabel("Terrain: ");
  QComboBox *tBox = new QComboBox();
  for(int i = 0; i < (int)terrain_t::COUNT; i++)
    { tBox->addItem(QString(toString((terrain_t)i).c_str())); }
  connect(tBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setTerrain(int)));
  tBox->setCurrentIndex((int)mOptions.terrain);
  QHBoxLayout *thbox = new QHBoxLayout();
  thbox->addWidget(tLabel);
  thbox->addWidget(tBox);
  QWidget *terrainWidget = new QWidget(this);
  terrainWidget->setLayout(thbox);

  QLabel *sLabel = new QLabel("Seed: ");
  QLineEdit *seedText = new QLineEdit();
  seedText->setText(QString::number(mOptions.seed));
  connect(seedText, SIGNAL(textChanged(const QString&)), this, SLOT(setSeed(const QString&)));
  QHBoxLayout *seed = new QHBoxLayout();
  seed->addWidget(sLabel);
  seed->addWidget(seedText);
  QWidget *seedWidget = new QWidget(this);
  seedWidget->setLayout(seed);

  QSpinBox *radSpin[3];
  for(int i = 0; i < 3; i++)
    {
      radSpin[i] = new QSpinBox();
      radSpin[i]->setRange(0, 24);
      radSpin[i]->setSingleStep(1);
      radSpin[i]->setSuffix(" chunks");
      radSpin[i]->setValue(mOptions.chunkRadius[i]);
    }
  connect(radSpin[0], SIGNAL(valueChanged(int)), this, SLOT(setChunkRadiusX(int)));
  connect(radSpin[1], SIGNAL(valueChanged(int)), this, SLOT(setChunkRadiusY(int)));
  connect(radSpin[2], SIGNAL(valueChanged(int)), this, SLOT(setChunkRadiusZ(int)));
  QGridLayout *rad = new QGridLayout();
  rad->addWidget(new QLabel("Chunk Radius: "), 1,0,1,1);
  for(int i = 0; i < 3; i++)
    {
      rad->addWidget(new QLabel(i == 0 ? "X" : (i==1 ? "Y" : "Z")), i,1,1,1);
      rad->addWidget(radSpin[i], i,2,1,2);
    }
  QWidget *radWidget = new QWidget(this);
  radWidget->setLayout(rad);

  QLabel *ltLabel = new QLabel("Load Threads: ");
  QSpinBox *ltSpin = new QSpinBox();
  ltSpin->setRange(1, 16);
  ltSpin->setSingleStep(1);
  ltSpin->setValue(mOptions.loadThreads);
  connect(ltSpin, SIGNAL(valueChanged(int)), this, SLOT(setLoadThreads(int)));
  QHBoxLayout *lt = new QHBoxLayout();
  lt->addWidget(ltLabel);
  lt->addWidget(ltSpin);
  QWidget *ltWidget = new QWidget(this);
  ltWidget->setLayout(lt);

  QLabel *mtLabel = new QLabel("Mesh Threads: ");
  QSpinBox *mtSpin = new QSpinBox();
  mtSpin->setRange(1, 16);
  mtSpin->setSingleStep(1);
  mtSpin->setValue(mOptions.meshThreads);
  connect(mtSpin, SIGNAL(valueChanged(int)), this, SLOT(setMeshThreads(int)));
  QHBoxLayout *mt = new QHBoxLayout();
  mt->addWidget(mtLabel);
  mt->addWidget(mtSpin);
  QWidget *mtWidget = new QWidget(this);
  mtWidget->setLayout(mt);

  QHBoxLayout *btnLayout = new QHBoxLayout();
  Button *backButton = new Button("Back");
  connect(backButton, SIGNAL(clicked()), this, SIGNAL(back()));
  Button *startButton = new Button("Create World");
  connect(startButton, SIGNAL(clicked()), this, SLOT(createWorld()));

  btnLayout->addWidget(backButton);
  btnLayout->addWidget(startButton);

  QVBoxLayout *innerLayout = new QVBoxLayout();
  innerLayout->setSpacing(5);
  innerLayout->setMargin(50);
  innerLayout->addWidget(nameWidget);
  innerLayout->addWidget(terrainWidget);
  innerLayout->addWidget(seedWidget);
  innerLayout->addWidget(radWidget);
  innerLayout->addWidget(ltWidget);
  innerLayout->addWidget(mtWidget);
  innerLayout->addLayout(btnLayout);
  
  QFrame *innerFrame = new QFrame();
  innerFrame->setFrameStyle(QFrame::Box);
  QPalette pal = innerFrame->palette();
  pal.setColor(QPalette::Window, QColor(Qt::black));
  innerFrame->setAutoFillBackground(true);
  innerFrame->setPalette(pal);
  innerFrame->update();
  innerFrame->setLayout(innerLayout);

  QHBoxLayout *hb = new QHBoxLayout();
  hb->addStretch(1);
  hb->addWidget(innerFrame);
  hb->addStretch(1);
  
  QVBoxLayout *mainLayout = new QVBoxLayout();
  mainLayout->addStretch(1);
  mainLayout->addLayout(hb);
  mainLayout->addStretch(1);
  setLayout(mainLayout);
}



void WorldCreate::setName(const QString &name)
{
  mOptions.name = name.toStdString();
}
void WorldCreate::setSeed(const QString &seed)
{
  mOptions.seed = (uint32_t)seed.toUInt();
}
void WorldCreate::setTerrain(int id)
{
  mOptions.terrain = (terrain_t)id;
}
void WorldCreate::setChunkRadiusX(int rx)
{
  mOptions.chunkRadius[0] = rx;
}
void WorldCreate::setChunkRadiusY(int ry)
{
  mOptions.chunkRadius[1] = ry;
}
void WorldCreate::setChunkRadiusZ(int rz)
{
  mOptions.chunkRadius[2] = rz;
}
void WorldCreate::setLoadThreads(int threads)
{
  mOptions.loadThreads = threads;
}
void WorldCreate::setMeshThreads(int threads)
{
  mOptions.meshThreads = threads;
}

void WorldCreate::createWorld()
{ emit created(mOptions); }
