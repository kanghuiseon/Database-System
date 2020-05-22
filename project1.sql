SELECT name
FROM Pokemon
WHERE type = 'Grass'
ORDER BY name ASC;

SELECT name
FROM Trainer
WHERE hometown = 'Brown City' OR hometown = 'Rainbow City'
ORDER BY name ASC;

SELECT DISTINCT type
FROM Pokemon
ORDER BY type ASC;

SELECT name
FROM City
WHERE name LIKE 'B%'
ORDER BY name ASC;

SELECT hometown
FROM Trainer
WHERE NOT (name LIKE 'M%')
ORDER BY hometown ASC;

SELECT nickname
FROM CatchedPokemon
WHERE level = (SELECT Max(level) FROM CatchedPokemon)
ORDER BY nickname ASC;

SELECT name
FROM Pokemon
WHERE (name LIKE 'A%' OR name LIKE 'I%' OR name LIKE 'U%' OR name LIKE 'E%' OR name LIKE 'O%')
ORDER BY name ASC;

SELECT AVG(level)
FROM CatchedPokemon;

SELECT MAX(level)
FROM CatchedPokemon
WHERE owner_id = (SELECT id from Trainer where name = 'Yellow');

SELECT distinct hometown
from Trainer
order by hometown ASC;

select Trainer.name, nickname
from Trainer, CatchedPokemon
where nickname like 'A%' AND owner_id = Trainer.id
order by Trainer.name ASC;

select Trainer.name
from Trainer
join City, Gym
where City.description = 'Amazon' and City.name = Trainer.hometown and Trainer.hometown = Gym.city and Gym.leader_id = Trainer.id;

select MAX(CatchedPokemon.owner_id), count(CatchedPokemon.id)
from Trainer, CatchedPokemon
join Pokemon
Where Pokemon.id = CatchedPokemon.pid and Pokemon.type = 'Fire' and CatchedPokemon.owner_id = Trainer.id;

select distinct type
from Pokemon
where Pokemon.id Like '_'
order by Pokemon.id DESC;

select count(id)
from Pokemon
where not type = 'Fire';

select Pokemon.name
from Pokemon
join Evolution
where Evolution.before_id > Evolution.after_id and Evolution.before_id = Pokemon.id
order by Pokemon.name ASC;

select avg(level)
from CatchedPokemon
join Pokemon, Trainer
where Pokemon.type = 'Water' and Pokemon.id = CatchedPokemon.pid;

select CatchedPokemon.nickname
from CatchedPokemon
where CatchedPokemon.owner_id in (select leader_id from Gym) and CatchedPokemon.level = (select max(level) from Catchedpokemon where CatchedPokemon.owner_id in (select leader_id from Gym));

select Trainer.name
from Trainer
where Trainer.id = (select owner_id from CatchedPokemon where owner_id in (select id from Trainer where Trainer.hometown = 'Blue City')
                    group by owner_id order by avg(level) desc limit 1)
order by Trainer.name asc;

select Pokemon.name
from Pokemon
where Pokemon.type = 'Electric' and Pokemon.id in (select before_id from evolution) and
Pokemon.id in (select pid from CatchedPokemon where owner_id in (select Trainer.id from Trainer where Trainer.hometown = 'Brown City' or Trainer.hometown = 'Rainbow City'));

select Trainer.name, sum(level)
from Trainer, CatchedPokemon
where Trainer.id in (select leader_id from Gym) and Trainer.id = CatchedPokemon.owner_id
group by Trainer.name
order by sum(level) desc;

select MAX(hometown)
from Trainer;

select distinct Pokemon.name
from Pokemon
where Pokemon.id in (select A.pid from CatchedPokemon as A
                     inner join CatchedPokemon as B
                     on A.pid = B.pid
                     where A.owner_id in (select Trainer.id from Trainer where Trainer.hometown = 'Brown City') and B.owner_id in (select Trainer.id from Trainer where Trainer.hometown = 'Sangnok City'));

select distinct Trainer.name
from Trainer
join CatchedPokemon, Pokemon
where Trainer.id = CatchedPokemon.owner_id and Pokemon.id = CatchedPokemon.pid and Pokemon.name like 'P%' and Trainer.hometown = 'Sangnok City'
order by Trainer.name asc;

select Trainer.name, Pokemon.name
from Trainer, Pokemon
join CatchedPokemon on Pokemon.id = CatchedPokemon.pid
where CatchedPokemon.owner_id = Trainer.id
order by Trainer.name asc, Pokemon.name asc;

select Pokemon.name
from Pokemon
where Pokemon.id in (select before_id from evolution where after_id not in (select before_id from Evolution) and before_id not in (select after_id from Evolution))
order by Pokemon.name asc;

select c.nickname
from CatchedPokemon as c
where owner_id in (select leader_id from Gym where city = 'Sangnok City') and pid in (select id from Pokemon where type = 'Water')
order by c.nickname asc;

select Trainer.name
from Trainer
where Trainer.id in (select owner_id from CatchedPokemon where pid in (select after_id from Evolution) group by owner_id having count(pid)>2)
order by Trainer.name asc;

select Pokemon.name
from Pokemon
where Pokemon.id not in (select CatchedPokemon.pid from CatchedPokemon)
order by Pokemon.name asc;

select max(level)
from CatchedPokemon, Trainer
where CatchedPokemon.owner_id = Trainer.id
group by hometown
order by max(level) desc;

select evl.before_id,
(select name from Pokemon where id in (select evl.before_id)) as first_name,
(select name from Pokemon where id in (select evl.after_id)) as second_name,
(select name from Pokemon where id in (select after_id from Evolution where before_id in (select evl.after_id))) as third_name
from Evolution evl
where evl.after_id in (select before_id from Evolution)
order by evl.before_id asc;
