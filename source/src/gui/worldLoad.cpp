#include "worldLoad.hpp"
#include "button.hpp"
#include "configFile.hpp"

#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QString>
#include <QPushButton>
#include <QListWidget>


WorldLoad::WorldLoad(World *world, QWidget *parent)
  : QWidget(parent), mWorld(world), mOptions(getDefaultOptions())
{
  mWorldList = new QListWidget();
  loadWorlds();
  connect(mWorldList, SIGNAL(currentRowChanged(int)), this, SLOT(selectWorld(int)));

  QLabel *nLabel = new QLabel("World Name: ");
  mNameLabel = new QLabel(mOptions.name.c_str());
  QHBoxLayout *name = new QHBoxLayout();
  name->addWidget(nLabel);
  name->addWidget(mNameLabel);
  QWidget *nameWidget = new QWidget(this);
  nameWidget->setLayout(name);
  
  QLabel *tLabel = new QLabel("Terrain: ");
  mTerrainLabel = new QLabel("");
  QHBoxLayout *thbox = new QHBoxLayout();
  thbox->addWidget(tLabel);
  thbox->addWidget(mTerrainLabel);
  QWidget *terrainWidget = new QWidget(this);
  terrainWidget->setLayout(thbox);
  
  QLabel *sLabel = new QLabel("Seed: ");
  mSeedLabel = new QLabel("");
  QHBoxLayout *seed = new QHBoxLayout();
  seed->addWidget(sLabel);
  seed->addWidget(mSeedLabel);
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
  Button *deleteButton = new Button("Delete World");
  connect(deleteButton, SIGNAL(clicked()), this, SLOT(deleteWorld()));
  Button *startButton = new Button("Load World");
  connect(startButton, SIGNAL(clicked()), this, SLOT(loadWorld()));

  btnLayout->addWidget(backButton);
  btnLayout->addWidget(deleteButton);
  btnLayout->addWidget(startButton);
  
  QVBoxLayout *innerLayout = new QVBoxLayout();
  innerLayout->setSpacing(5);
  innerLayout->setMargin(50);
  innerLayout->addWidget(mWorldList);
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

void WorldLoad::update()
{
  writeDefaultOptions(mOptions);
}

void WorldLoad::setChunkRadiusX(int rx)
{
  mOptions.chunkRadius[0] = rx;
  update();
}
void WorldLoad::setChunkRadiusY(int ry)
{
  mOptions.chunkRadius[1] = ry;
  update();
}
void WorldLoad::setChunkRadiusZ(int rz)
{
  mOptions.chunkRadius[2] = rz;
  update();
}
void WorldLoad::setLoadThreads(int threads)
{
  mOptions.loadThreads = threads;
  update();
}
void WorldLoad::setMeshThreads(int threads)
{
  mOptions.meshThreads = threads;
  update();
}

void WorldLoad::loadWorld()
{ emit loaded(mOptions); }
void WorldLoad::deleteWorld()
{
  int wIndex = mWorldList->currentRow();
  LOGDC(COLOR_GREEN, "Prev index: %d", wIndex);
  emit deleted(mOptions.name);
  refreshList();
  LOGDC(COLOR_GREEN, "Num worlds: %d", mWorlds.size());
  selectWorld(std::min(std::max(0, wIndex), (int)mWorlds.size()-1));
  //loadWorlds();
}

void WorldLoad::refreshList()
{
  loadWorlds();
}

void WorldLoad::selectWorld(int index)
{
  if(index >= 0 && index < mWorlds.size())
    {
      LOGDC(COLOR_CYAN, "Selecting world: %d", index);
      mOptions.name = mWorlds[index].name;
      mOptions.terrain = mWorlds[index].terrain;
      mOptions.seed = mWorlds[index].seed;
  
      mNameLabel->setText(mOptions.name.c_str());
      mTerrainLabel->setText(QString(toString(mOptions.terrain).c_str()));
      mSeedLabel->setText(QString::number(mOptions.seed));

      mWorldList->setCurrentRow(index); //item(index)->setSelected(true);
    }
}

void WorldLoad::loadWorlds()
{
  mWorldList->clear();
  mWorlds = mWorld->getWorlds();
  for(auto w : mWorlds)
    { mWorldList->addItem(w.name.c_str()); }
}
