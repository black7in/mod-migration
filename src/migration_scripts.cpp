/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */
#include "ServerSystem.h"


struct SpellTier {
    int requiredLevel;
    uint32 spellId;
};

class TeleportMigrationPlayer : public BasicEvent
{
public:
    TeleportMigrationPlayer(Player* player) : _player(player) { }

    bool Execute(uint64 /*eventTime*/, uint32 /*updateTime*/) override {
        _player->TeleportTo(1, -10739.21, 2442.38, 7.700, 4.99);
        return true;
    }

private:
    Player* _player;
};

void Azerothcore_skip_deathknight_HandleSkip(Player* player)
{

    ChatHandler(player->GetSession()).SendNotification("Hubo un error al migrar tu personaje, contacta un administrador.");
    //Not sure where DKs were supposed to pick this up from, leaving as the one manual add
    //player->AddItem(6948, true); //Hearthstone

    // these are all the starter quests that award talent points, quest items, or spells
    int STARTER_QUESTS[33] = { 12593, 12619, 12842, 12848, 12636, 12641, 12657, 12678, 12679, 12680, 12687, 12698, 12701, 12706, 12716, 12719, 12720, 12722, 12724, 12725, 12727, 12733, -1, 12751, 12754, 12755, 12756, 12757, 12779, 12801, 13165, 13166 };

    int specialSurpriseQuestId = -1;
    switch (player->getRace())
    {
    case RACE_TAUREN:
        specialSurpriseQuestId = 12739;
        break;
    case RACE_HUMAN:
        specialSurpriseQuestId = 12742;
        break;
    case RACE_NIGHTELF:
        specialSurpriseQuestId = 12743;
        break;
    case RACE_DWARF:
        specialSurpriseQuestId = 12744;
        break;
    case RACE_GNOME:
        specialSurpriseQuestId = 12745;
        break;
    case RACE_DRAENEI:
        specialSurpriseQuestId = 12746;
        break;
    case RACE_BLOODELF:
        specialSurpriseQuestId = 12747;
        break;
    case RACE_ORC:
        specialSurpriseQuestId = 12748;
        break;
    case RACE_TROLL:
        specialSurpriseQuestId = 12749;
        break;
    case RACE_UNDEAD_PLAYER:
        specialSurpriseQuestId = 12750;
        break;
    }

    STARTER_QUESTS[22] = specialSurpriseQuestId;
    STARTER_QUESTS[32] = player->GetTeamId() == TEAM_ALLIANCE ? 13188 : 13189;

    for (int questId : STARTER_QUESTS)
    {
        if (player->GetQuestStatus(questId) == QUEST_STATUS_NONE)
        {
            player->AddQuest(sObjectMgr->GetQuestTemplate(questId), nullptr);
            player->RewardQuest(sObjectMgr->GetQuestTemplate(questId), 0, player, false);
        }
    }

    //these are alternate reward items from quest 12679, item 39320 is chosen by default as the reward
    player->AddItem(38664, true);//Sky Darkener's Shroud of the Unholy
    player->AddItem(39322, true);//Shroud of the North Wind

    //these are alternate reward items from quest 12801, item 38633 is chosen by default as the reward
    player->AddItem(38632, true);//Greatsword of the Ebon Blade

    int DKL = 80;
    if (player->GetLevel() <= DKL)
    {
        //GiveLevel updates character properties more thoroughly than SetLevel
        player->GiveLevel(DKL);
    }


        player->addSpell(49998, SPEC_MASK_ALL, true); // Death Strike rank 1
        player->addSpell(47528, SPEC_MASK_ALL, true); // Mind Freeze
        player->addSpell(46584, SPEC_MASK_ALL, true); // Raise Dead
        player->addSpell(45524, SPEC_MASK_ALL, true); // Chains of Ice
        player->addSpell(48263, SPEC_MASK_ALL, true); // Frost Presence
        player->addSpell(50842, SPEC_MASK_ALL, true); // Pestilence
        player->addSpell(53342, SPEC_MASK_ALL, true); // Rune of Spellshattering
        player->addSpell(48721, SPEC_MASK_ALL, true); // Blood Boil rank 1
        player->addSpell(54447, SPEC_MASK_ALL, true); // Rune of Spellbreaking

    //Don't need to save all players, just current
    player->SaveToDB(false, false);

    player->m_Events.AddEvent(new TeleportMigrationPlayer(player), player->m_Events.CalculateTime(3000));
}

// Add player scripts
class MyPlayerMigration : public PlayerScript
{
public:
    MyPlayerMigration() : PlayerScript("MyPlayerMigration") { }

    void OnPlayerLogin(Player* player) override
    {
        if (player->GetLevel() > 1)
            return;

        if(player->GetLevelUpType() != LEVEL_TYPE_NORMAL)
            return;

        uint32 guid = player->GetGUID().GetCounter();
        QueryResult result = CharacterDatabase.Query("SELECT * FROM characters_migration_data WHERE guid = {} AND status = 0;", guid);
        if (result)
        {
            if (player->getClass() == CLASS_DEATH_KNIGHT) {
                Azerothcore_skip_deathknight_HandleSkip(player);
            }else
                player->m_Events.AddEvent(new TeleportMigrationPlayer(player), player->m_Events.CalculateTime(3000));
        }
    }
};

class MigrationNPC : public CreatureScript {
public:
    MigrationNPC() : CreatureScript("MigrationNPC") { }

    bool OnGossipHello(Player* player, Creature* creature) override {
        ClearGossipMenuFor(player);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Reclamar migración", 0, 1);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Adios", 0, 4);

        sServerSystem->SetGossipText(player, "Saludos $N, si estás aqui es porque te aprobaron tu migración y ya puedes reclamarla. Hazlo una sola vez y revisa tu correo. Suerte y bienvenido a Murloc Wow.\nNota: Si tu personaje tiene algún progreso o elegiste algun reto de leveo entonces la migración será cancelada y deberas ponerte en contacto con un administrador.", 600410);
        SendGossipMenuFor(player, 600410, creature);
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action) override {
        ClearGossipMenuFor(player);
        switch (action) {
        case 1: {
            if (player->getClass() != CLASS_DEATH_KNIGHT && player->GetLevel() > 1) {
                ChatHandler(player->GetSession()).SendNotification("Debes ser nivel 1 para poder reclamar la migración.");
                CloseGossipMenuFor(player);
                return true;
            }

            if(player->getClass() == CLASS_DEATH_KNIGHT && player->GetLevel() > 55) {
                ChatHandler(player->GetSession()).SendNotification("Debes ser nivel 55 para poder reclamar la migración.");
                CloseGossipMenuFor(player);
                return true;
            }

            if (player->getClass() == CLASS_DEATH_KNIGHT) {
                ChatHandler(player->GetSession()).SendNotification("Hubo un error al migrar tu personaje, contacta un administrador.");
                CloseGossipMenuFor(player);
                return true;
            }

            if (player->GetLevelUpType() != LEVEL_TYPE_NORMAL) {
                ChatHandler(player->GetSession()).SendNotification("Tu personaje ya tiene un progreso, por favor contacta un administrador.");
                CloseGossipMenuFor(player);
                return true;
            }

            // Check char database, table characters_migration_data if exist guid
            uint32 guid = player->GetGUID().GetCounter();
            QueryResult result = CharacterDatabase.Query("SELECT * FROM characters_migration_data WHERE guid = {} AND status = 0;", guid);
            if (result)
            {
                //migrationId, guid, level, money, arena, honor, currency, item, mount, title, achievement, reputation, profesion
                Field* fields = result->Fetch();
                uint32 migrationId = fields[0].Get<uint32>();
                uint32 level = fields[2].Get<uint32>();
                uint32 money = fields[3].Get<uint32>();
                uint32 arena = fields[4].Get<uint32>();
                uint32 honor = fields[5].Get<uint32>();
                std::string currency = fields[6].Get<std::string>();
                std::string item = fields[7].Get<std::string>();
                std::string mount = fields[8].Get<std::string>();
                std::string title = fields[9].Get<std::string>();
                std::string achievement = fields[10].Get<std::string>();
                std::string reputation = fields[11].Get<std::string>();
                std::string profesions = fields[12].Get<std::string>();

                player->SetLevel(level);
                player->SetMoney(money);
                //player->SetArenaPoints(arena);
                //player->SetHonorPoints(honor);

                auto currencyArray = parseItems(currency);
                auto splitCurrencyArray = splitItemsByStack(currencyArray);
                enviarItemsEnCorreos(player, splitCurrencyArray);

                auto itemsArray = parseItems(item);
                auto splitItemsArray = splitItemsByStack(itemsArray);
                enviarItemsEnCorreos(player, splitItemsArray);

                auto mounts = parseMounts(mount);
                enviarMonturasEnCorreos(player, mounts);
                /*for (int mountId : mounts) {
                    if (!player->HasSpell(mountId))
                        player->learnSpell(mountId, false);
                }*/

                /*auto titles = parseMounts(title);
                for (int titleId : titles) {
                    CharTitlesEntry const* t = sCharTitlesStore.LookupEntry(titleId);
                    if (t)
                        player->SetTitle(t, false);
                }*/

                auto repuArray = parseItems(reputation);
                for (auto& [entry, qty] : repuArray) {
                    //std::cout << "Entry: " << entry << " -> Cantidad: " << qty << std::endl;
                    if (player->GetReputation(entry) < qty)
                        player->SetReputation(entry, qty);
                }

                processProfessions(player, profesions);

                /*auto achievementArray = parseMounts(achievement);
                for (int achievementId : achievementArray) {
                    if (!player->HasAchieved(achievementId)) {
                        AchievementEntry const* a = sAchievementStore.LookupEntry(achievementId);
                        player->GetAchievementMgr()->CompletedAchievement(a);
                    }
                }*/

                player->SetLevelUpType(LEVEL_TYPE_MIGRATION);
                //update status = 1
                CharacterDatabase.Execute("UPDATE characters_migration_data SET status = 1 WHERE guid = {} AND migrationId = {};", guid, migrationId);
            }
            CloseGossipMenuFor(player);
            break;
        }
        case 4:
            creature->Say("Adios!, espero verte pronto.", LANG_UNIVERSAL);
            CloseGossipMenuFor(player);
        default:
            CloseGossipMenuFor(player);
            break;
        }
        return true;
    }

private:
    std::vector<std::pair<int, int>> parseItems(const std::string& data) {
        std::vector<std::pair<int, int>> items;
        std::stringstream ss(data);
        std::string pair;

        // Separar por ';'
        while (std::getline(ss, pair, ';')) {
            size_t pos = pair.find(':');
            if (pos != std::string::npos) {
                int entry = std::stoi(pair.substr(0, pos));         // parte antes de ':'
                int quantity = std::stoi(pair.substr(pos + 1));     // parte después de ':'
                items.emplace_back(entry, quantity);
            }
        }
        return items;
    }

    std::vector<int> parseMounts(const std::string& data) {
        std::vector<int> mounts;
        std::stringstream ss(data);
        std::string entry;

        while (std::getline(ss, entry, ';')) {
            if (!entry.empty()) {
                mounts.push_back(std::stoi(entry));
            }
        }
        return mounts;
    }

    std::vector<std::pair<int, int>> splitItemsByStack(
        const std::vector<std::pair<int, int>>& itemsArray
    ) {
        std::vector<std::pair<int, int>> result;

        for (auto& [entry, qty] : itemsArray) {
            ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(entry);
            if (!itemTemplate) {
                continue;
            }

            uint32 stackable = itemTemplate->Stackable;
            if (stackable == 0) stackable = 1; // por si acaso

            int remaining = qty;
            while (remaining > 0) {
                int amount = (remaining >= stackable) ? stackable : remaining;
                result.emplace_back(entry, amount);
                remaining -= amount;
            }
        }

        return result;
    }

    void enviarItemsEnCorreos(Player* player, const std::vector<std::pair<int, int>>& itemsArray) {
        const uint32 MAX_ITEMS_PER_MAIL = 12;
        const std::string subject = "Migración";
        const std::string text = "Felicidades, tu migración fue aceptada y tus items los recibes en este correo.";
        const uint32 senderGUIDLow = 0;
        const uint32 stationary = 61;
        const uint32 delay = 0;
        const uint32 money = 0;
        const uint32 cod = 0;

        size_t totalItems = itemsArray.size();
        size_t index = 0;

        while (index < totalItems) {
            MailSender sender(MAIL_NORMAL, senderGUIDLow, (MailStationery)stationary);
            MailDraft draft(subject, text);

            if (cod)
                draft.AddCOD(cod);
            if (money)
                draft.AddMoney(money);

            CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();

            // Agregar hasta 12 ítems
            for (size_t count = 0; count < MAX_ITEMS_PER_MAIL && index < totalItems; ++count, ++index) {
                uint32 entry = itemsArray[index].first;
                uint32 amount = itemsArray[index].second;

                if (Item* item = Item::CreateItem(entry, amount)) {
                    item->SaveToDB(trans);
                    draft.AddItem(item);
                }
            }

            // Enviar el correo
            draft.SendMailTo(trans, MailReceiver(player, player->GetGUID().GetCounter()), sender, MAIL_CHECK_MASK_NONE, delay);
            CharacterDatabase.CommitTransaction(trans);
        }
    }

    void enviarMonturasEnCorreos(Player* player, const std::vector<int>& itemIds) {
        const uint32 MAX_ITEMS_PER_MAIL = 12;
        const std::string subject = "Migración";
        const std::string text = "Felicidades, tu migración fue aceptada y tus items los recibes en este correo.";
        const uint32 senderGUIDLow = 0;
        const uint32 stationary = 61;
        const uint32 delay = 0;
        const uint32 money = 0;
        const uint32 cod = 0;

        size_t totalItems = itemIds.size();
        size_t index = 0;

        while (index < totalItems) {
            MailSender sender(MAIL_NORMAL, senderGUIDLow, (MailStationery)stationary);
            MailDraft draft(subject, text);

            if (cod)
                draft.AddCOD(cod);
            if (money)
                draft.AddMoney(money);

            CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();

            // Agregar hasta 12 ítems
            for (size_t count = 0; count < MAX_ITEMS_PER_MAIL && index < totalItems; ++count, ++index) {
                uint32 entry = static_cast<uint32>(itemIds[index]);
                uint32 amount = 1;

                if (Item* item = Item::CreateItem(entry, amount)) {
                    item->SaveToDB(trans);
                    draft.AddItem(item);
                }
            }

            // Enviar el correo
            draft.SendMailTo(trans, MailReceiver(player, player->GetGUID().GetCounter()), sender, MAIL_CHECK_MASK_NONE, delay);
            CharacterDatabase.CommitTransaction(trans);
        }
    }

    void learnProfessionSpells(Player* player, const std::string& profession, int level) {
        // Convertir a minúsculas
        std::string prof = profession;
        std::transform(prof.begin(), prof.end(), prof.begin(), ::tolower);

        // Diccionario: profesión -> lista de {nivel requerido, id de hechizo}
        static const std::unordered_map<std::string, std::vector<SpellTier>> professionSpells = {
            {"primeros auxilios", {{75,3273},{150,3274},{225,7924},{300,10846},{375,27028},{450,45542}}},
            {"herrería", {{75,2018},{150,3100},{225,3538},{300,9785},{375,29844},{450,51300}}},
            {"peletería", {{75,2108},{150,3104},{225,3811},{300,10662},{375,32549},{450,51302}}},
            {"alquimia", {{75,2259},{150,3101},{225,3464},{300,11611},{375,28596},{450,51304}}},
            {"herboristería", {{75,2366},{150,2368},{225,3570},{300,11993},{375,28695},{450,50300}}},
            {"cocina", {{75,2550},{150,3102},{225,3413},{300,18260},{375,33359},{450,51296}}},
            {"minería", {{75,2575},{150,2576},{225,3564},{300,10248},{375,29354},{450,50310}}},
            {"sastrería", {{75,3908},{150,3909},{225,3910},{300,12180},{375,26790},{450,51309}}},
            {"ingeniería", {{75,4036},{150,4037},{225,4038},{300,12656},{375,30350},{450,51306}}},
            {"encantamiento", {{75,7411},{150,7412},{225,7413},{300,13920},{375,28029},{450,51313}}},
            {"pesca", {{75,7620},{150,7731},{225,7732},{300,18248},{375,33095},{450,51294}}},
            {"desollar", {{75,8613},{150,8617},{225,8618},{300,10768},{375,32678},{450,50305}}},
            {"joyería", {{75,25229},{150,25230},{225,28894},{300,28895},{375,28897},{450,51311}}},
            {"inscripción", {{75,45357},{150,45358},{225,45359},{300,45360},{375,45361},{450,45363}}}
        };

        auto it = professionSpells.find(prof);
        if (it != professionSpells.end()) {
            for (const auto& tier : it->second) {
                if (level >= tier.requiredLevel) {
                    player->learnSpell(tier.spellId);
                }
            }
        }
    }

    void processProfessions(Player* player, const std::string& data) {
        static const std::unordered_map<std::string, uint32> professionSkillIds = {
            {"primeros auxilios", 129},
            {"cocina", 185},
            {"pesca", 356},
            {"inscripción", 773},
            {"joyería", 755},
            {"peletería", 165},
            {"herrería", 164},
            {"alquimia", 171},
            {"encantamiento", 333},
            {"ingeniería", 202},
            {"sastrería", 197},
            {"minería", 186},
            {"desollar", 393},
            {"herboristería", 182}
        };

        std::stringstream ss(data);
        std::string token;

        while (std::getline(ss, token, ';')) {
            std::stringstream part(token);
            std::string profName, lvlStr, ptsStr;

            if (std::getline(part, profName, ':') &&
                std::getline(part, lvlStr, ':') &&
                std::getline(part, ptsStr, ':'))
            {
                // Convertir a minúsculas para búsqueda
                std::transform(profName.begin(), profName.end(), profName.begin(), ::tolower);

                int level = std::stoi(lvlStr);
                int puntos = std::stoi(ptsStr);

                // Aprender la profesión y sus rangos de hechizos
                learnProfessionSpells(player, profName, level);

                // Buscar el skillId correspondiente
                auto it = professionSkillIds.find(profName);
                if (it != professionSkillIds.end()) {
                    uint32 skillId = it->second;
                    bool targetHasSkill = player->GetSkillValue(skillId);

                    player->SetSkill(skillId, targetHasSkill ? player->GetSkillStep(skillId) : 1, puntos, level);
                    // Setear puntos en el skill
                    //player->SetSkill(skillId, level, puntos);
                    // Nota: Dependiendo de tu core, podría ser player->SetSkill(skillId, puntos, puntos);
                }
            }
        }
    }

};




// Add all scripts in one
void AddMyPlayerMigrationScripts()
{
    new MyPlayerMigration();
    new MigrationNPC();
}
