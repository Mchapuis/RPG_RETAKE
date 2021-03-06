
#include "stdafx.h"
#include "character.h"
#include "map.h"
#include <algorithm>//used for sort elements in abilities array

//BOOST_CLASS_EXPORT_GUID(GameCharacter, "GameCharacter")
BOOST_CLASS_EXPORT_GUID(Player, "Player")
//BOOST_CLASS_EXPORT_GUID(NPC, "NPC")
BOOST_CLASS_EXPORT_GUID(Enemy, "Enemy")
BOOST_CLASS_EXPORT_GUID(Friendly, "Friendly")

const std::string GameCharacter::symbol = "G";
const std::string Player::symbol = "P";
const std::string NPC::symbol = "N";
const std::string Enemy::symbol = "X";
const std::string Friendly::symbol = "F";

//! Character parametrized constructor.
//! Takes a name and an initial level.
//! The assigned level generates a character with according stats.
GameCharacter::GameCharacter(string aName, int aBaseAtk, int aLevel) :
name(aName), level(0), baseAtk(aBaseAtk), Hp(0), MaxHp(0), inventory(Inventory())
{
	int i;
	NotifyCharacter("Creating Character " + aName);
	abilities = abilitiesRoll();
	updateBonus();
	//modifiers = std::vector<int>(Ability::getCount());

	if (aLevel < 0) aLevel == 0;

	this->updateLvl(aLevel);
}

//Assume ability rools of 4d6 drop lowest, and 1d10 Hit Die. 
vector<int> GameCharacter::abilitiesRoll()
{
	int SIZE = 6;
	vector<Ability> abls = Ability::getMainAbls();
	vector<int> result = vector<int>(SIZE);
	Logger ablLog("Abilities roll logger");
	std::string myString = " ";

	ablLog.Update("Abilities Roll");

	//Take all the randoms numbers
	for (int i = 0; i < SIZE; i++){
		result[i] = abilityRoll();
		myString += to_string(result[i]) + " ";
	}

	ablLog.Update("rolls: " + myString);

	//order the elements in the array
	std::sort(result.begin(), result.end());

	//decreasing order...
	std::reverse(result.begin(), result.end());

	myString = "";
	for (int i = 0; i < SIZE; i++){
		myString += to_string(result[i]) + " ";
	}

	ablLog.Update("Sorted rolls: " + myString);

	//put numbers in the Ability const array...
	int i = 0;
	for (const Ability a : abls)
	{
		result[a.index] = result[i];
		ablLog.Update(a.abbr + ": " + to_string(result[a.index]));
		//cout << "result:::" << result[i] << endl;
		i++;
	}
	return result;
}

//! Using 4d6 drop lowest rule.
int GameCharacter::abilityRoll()
{
	vector<int> dices = vector<int>();
	int result = 0;
	int i;
	int roll;
	std::string myString = "";

	Logger ablLog("Ability roll");

	for (i = 0; i < 4; i++)
	{
		roll = Dice::roll(1, 6);
		ablLog.Update("Roll " + to_string(i + 1) + " : " + to_string(roll));
		dices.push_back(roll);
	}

	sort(dices.begin(), dices.end());

	myString += "Total roll: " + to_string(dices.back());

	for (i = 0; i < 3; i++)
	{
		myString += " + " + to_string(dices.back());
		result += dices.back();
		dices.pop_back();
	}

	ablLog.Update(myString + to_string(result));

	return result;
}

void GameCharacter::updateBonus()
{
	vector<Ability> mainAbls = Ability::getMainAbls();
	vector<int> result = vector<int>(abilities.size());
	int i;

	for (Ability a: mainAbls)
	{
		result[a.index] = calcBonus(abilities[a.index]);
	}

	bonus = result;
}

//! Function for a single level increment.
/*! The function gives the character an ability 
    point every 4 levels, as per D&D rules (here randomly)*/
void GameCharacter::levelUp()
{
	int i;
	int temp;
	
	level++;
	NotifyCharacter("Character " + this->getName() + " leveling up to lvl " + to_string(this->getLevel()));

	//randomly increase an ability bonus each 4 levels.
	if (level % 4 == 0)
	{
		i = rand() % 6;
		abilities[Ability::getMainAbls()[i].index]++;
		temp = abilities[Ability::getMainAbls()[i].index];
		NotifyCharacter("Base Ability " + Ability::getMainAbls()[i].name + " raised to " + to_string(temp));
		NotifyCharacter(Ability::getMainAbls()[i].name + " modifier is now " + to_string(calcBonus(temp)));
	}

	updateBonus();
	temp = MaxHp;
	MaxHp += this->hitDie();
	Hp = MaxHp;
}

//! Function to set a characters level.
/*! The function calls on GameCharacter::levelUP to
    ensure the character stats move up accordingly*/
void GameCharacter::updateLvl(int toLevel)
{
	NotifyMap("Update character " + this->getName() + " to level " + to_string(toLevel));

	if (toLevel < 0) toLevel = 0;

	if (level == 0 && toLevel > 0) inventory.updateLvl(toLevel);
	
	while (level < toLevel)	this->levelUp();
}

int GameCharacter::hitDie()
{
	int die;

	if (level == 1)
	{
		die = 10;
	}

	die = Dice::roll(1, 10);
	NotifyCharacter("Hit die roll: " + to_string(die));
	NotifyCharacter("MaxHp increased of " + to_string(die + bonus[Ability::CON.index]));
	return die + bonus[Ability::CON.index];
}

int GameCharacter::getRange()
{
	Weapon* weapon;

	if (this->inventory.getEquipType(EquipType::WEAPON) != NULL)
	{
		weapon = (Weapon*) this->inventory.getEquipType(EquipType::WEAPON);
		return weapon->getRange();
	}

	return -1;
}

int GameCharacter::damageRoll(int distance)
{
	Weapon* weapon = (Weapon*) this->inventory.getEquipType(EquipType::WEAPON);
	pair<int, int> dmg;
	int range;
	int str;
	int dext;
	int roll;

	if (!weapon)
	{
		return 0;
	}
	else
	{

		dmg = weapon->getDamage();
		range = weapon->getRange();
		str = this->getNetStat(Ability::STR);
		dext = this->getNetStat(Ability::DEX);
	
		if (range > 1 && str > 0) str = 0;

		if (range <= 1)
		{
			dext = 0;

			if (distance > 1) return 0; //Assume their is no long-range melee weapons
		}

		roll = Dice::roll(dmg.first, dmg.second);
		roll += str + dext;

		if (range > 1 && distance > range) roll -= 2 * (distance - range);

		return roll;
	}
}

//!Give turn to a character
//!@param: objects is a list of paired key values (example: looking for a character and a cell is an object)
bool GameCharacter::startTurn(Map* map, std::map<Placeable*, Cell*> *objects)
{
	//object map is not used anymore, but do not remove yet! :) Optimize later plz!
	return strategy->turn(objects);
}



unordered_set<Item*> GameCharacter::unlock(Lockable* lock)
{
	std::unordered_set<KeyItem*> keyChain;

	if (!lock->isLocked()) return lock->removeAll();

	for (KeyItem* k : keyChain)
	{
		if (lock->unlock(*k))
		{
			keyChain.erase(k);
			delete k;
			return lock->removeAll();
		}
	}

	return std::unordered_set<Item*>();
}

void GameCharacter::attack(GameCharacter* opponent, int distance)
{
	int attackCount = this->numOfAttacks();
	int d20;
	int atk_bonus;
	int i;

	if (distance > 10) return;

	//cout << this->name << " attacks " << opponent->name << '\n';

	//Loop through character number of attacks.
	for (i = 1; i <= attackCount; i++)
	{
		atk_bonus = this->ATK_Bonus(i);

		// Roll 1d20
		d20 = Dice::roll(1, 20);

		//cout << i << "Dice roll = " << d20 << '\n';
		//cout << i << "Opponent AC = " << opponent->AC() << '\n';

		// If roll 1, miss.
		if (d20 == 1) continue;

		//If roll 20, hit, and possibly critical.
		if (d20 == 20)
		{
			//Attack roll to see if critical hit.
			if (Dice::roll(1, 20, ATK_Bonus(i)) >= opponent->AC())
			{
				opponent->takeDamage(this, this->damageRoll(distance) * 2);
			}

			//Normal hit if not.
			opponent->takeDamage(this, this->damageRoll(distance));
		}
		//Complete standard attack roll if between 2 and 18.
		else if (d20 + ATK_Bonus(i) >= opponent->AC())
		{
			int damage = this->damageRoll(distance);

			//cout << i << "damage roll: " << damage << endl;
			opponent->takeDamage(this, damage);
			//cout << i << "opponent hp: " << opponent->Hp << "/" << opponent->MaxHp <<  endl;
		}
	}

}

int GameCharacter::modifyHp(int variation)
{
	Hp = (Hp - variation <= 0) ? Hp == 0 : Hp - variation;

	return Hp;
}

//! Function that returns a single base ability of a character.
int GameCharacter::getBaseAbl(Ability abl)
{
	return abilities[abl.index];
}

//! Function that sets a single requested base ability of a character.
void GameCharacter::setBaseAbl(Ability abl, int value)
{
	abilities[abl.index] = value;
}

//! Function that returns an array of a character
//! ability bonus.
int* GameCharacter::getAllBonus()
{
	int* result = new int[6];
	int i;

	for (i = 0; i < 6; i++)
	{
		result[i] = bonus[i];
	}

	return result;
}

//! Function that returns a single ability bonus of a character.
int GameCharacter::getBonus(Ability abl)
{
	return calcBonus(abilities[abl.index]);
}

int GameCharacter::ATK_Bonus(int NthAttack)
{
	if (NthAttack > 0 && NthAttack <= numOfAttacks())
	{ 
		return (level + baseAtk - 1) - (numOfAttacks() - NthAttack) * 5;
	}

	return 0;
}

//! Function that returns an array of a character
//! net ability scores, with bonus and modifiers included
int* GameCharacter::getAllNetStats()
{
	int* result = new int[6];
	int* enchantments = inventory.getAllEquipMods();
	int i;

	for (i = 0; i < 6; i++)
	{
		result[i] = abilities[i] + enchantments[i];
		//result[i] = abilities[i] + modifiers[i] + itemModifiers[i];
	}

	return result;
}

//! Function that returns a single net ability score
//! of a character, with bonus and modifiers included.
int GameCharacter::getNetStat(Ability abl)
{
	return this->calcBonus(abl)
		+ inventory.getEquipMod(abl);
}

std::string GameCharacter::toString()
{
	std::string result = "";
	std::string temp = "";
	int i;
	int j;

	result += " " + name + '\n';
	result += "Lvl:" + to_string(level) + " ";
	result += "HP:" + to_string(Hp) + "/" + to_string(MaxHp);
	
	result += "        "; //spaces for the abilities and modifiers clarity

	for (i = 0; i < abilities.size(); i++)
	{
		temp = Ability::get(i).abbr;

		while (temp.size() < 1) temp += " ";
		temp += ":";
		temp += to_string(abilities[i]);

		if (bonus[i] != 0)
		{
			temp += "/" + to_string(bonus[i]);
		}

		result += temp + "  ";
	}

	return (result + inventory.toString());
}

bool NPC::reset()
{ 
	this->resetLevel();
	return true;
}

Player Player::sLoad(std::string filename)
{
	Player result = Player();
	(&result)->load(filename);
	return result;
}

void Player::save(std::string filename)
{
	std::ofstream ofs(ASSETS_PATH + "characters\\" + filename);
	boost::archive::text_oarchive oa(ofs);
	oa << *this;
}

void Player::load(std::string filename)
{
	std::ifstream ifs(ASSETS_PATH + "characters\\" + filename);
	boost::archive::text_iarchive ia(ifs);
	ia >> *this;
}

Enemy Enemy::sLoad(std::string filename)
{
	Enemy result = Enemy();
	(&result)->load(filename);
	return result;
}

void Enemy::save(std::string filename)
{
	std::ofstream ofs(ASSETS_PATH + "characters\\" + filename);
	boost::archive::text_oarchive oa(ofs);
	oa << *this;
}

void Enemy::load(std::string filename)
{
	std::ifstream ifs(ASSETS_PATH + "characters\\" + filename);
	boost::archive::text_iarchive ia(ifs);
	ia >> *this;
}

Friendly Friendly::sLoad(std::string filename)
{
	Friendly result = Friendly();
	(&result)->load(filename);
	return result;
}

void Friendly::save(std::string filename)
{
	std::ofstream ofs(ASSETS_PATH + "characters\\" + filename);
	boost::archive::text_oarchive oa(ofs);
	oa << *this;
}

void Friendly::load(std::string filename)
{
	std::ifstream ifs(ASSETS_PATH + "characters\\" + filename);
	boost::archive::text_iarchive ia(ifs);
	ia >> *this;
}