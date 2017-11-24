#include "mem.h"
#include "common.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

mem_fit_function_t * fonction_recherche;

//Descripteurs de fichiers
int requested_sizes;
int real_sizes;
unsigned int index;

//Variables utilisées pour compter les tailles réelles
size_t real_size;
size_t requested_size;

void write_integer(int file, size_t integer){
	int cpt = 0;
	int integer_tmp = integer;
	while (integer_tmp > 0){
		cpt++;
		integer_tmp = integer_tmp/10;
	}

	
}

//Ecriture des statistique dans des fichiers
void update_stats(size_t requested_size, size_t real_size){
	
	//dprintf("requested_size : %lu\n", requested_size);
	//dprintf("real_size : %lu\n", real_size);
	//Changer l'index en caractere
	int dec=1;
	int number=index;
	while(number > 9){
		dec++;
		number/=10;
	}
	char buf[dec];
	snprintf(buf,sizeof(int),"%d",index);

	//Changer le nombre requis en caractere
	dec=1;
	int numberRequested=requested_size;
	while(numberRequested > 9){
		dec++;
		numberRequested/=10;
	}
	char bufRequested[dec];
	snprintf(bufRequested,sizeof(size_t),"%lu",requested_size);
	
	//Changer le nombre reel en caractere
	dec=1;
	int numberReal=real_size;
	while(numberReal > 9){
		dec++;
		numberReal/=10;
	}
	char bufReal[dec];
	snprintf(bufReal,sizeof(size_t),"%lu",real_size);
	
	
	if (requested_sizes == -1 || real_sizes == -1)
	    exit(1);

	//Debut des ecritures
	write(requested_sizes, buf, sizeof(buf));
	write(requested_sizes, (void*)";", sizeof(";"));
	write(requested_sizes, bufRequested, sizeof(bufRequested));
	write(requested_sizes,"\n",sizeof("\n"));
	write(real_sizes, (void*)&buf, sizeof(buf));
	write(real_sizes, (void*)";", sizeof(";"));
	write(real_sizes, bufReal, sizeof(bufReal));
	write(real_sizes,"\n",sizeof("\n"));

	index++;
}

//Initialisation de l'allocateur : on cree le premier fb et on met a jour l'adresse presente en debut de memoire avec l'adresse du nouveau fb
void mem_init(char* mem, size_t taille){
        *(struct fb**) mem =  (void*)mem + sizeof(struct fb*);
        struct fb* first_free_zone = *(struct fb**) mem ;
        first_free_zone->size = taille-sizeof(struct fb*);
        first_free_zone->next = NULL;
        //printf("Taille : %lu , Adresse de la premiere zone libre: %p\n",(size_t)first_free_zone->size,first_free_zone);
        mem_fit(&mem_fit_worst);
	requested_sizes = open("requested_sizes.txt", O_WRONLY | O_CREAT, S_IRWXU | S_IRGRP | S_IWGRP | S_IROTH);
	real_sizes = open("real_sizes.txt", O_WRONLY | O_CREAT, S_IRWXU | S_IRGRP | S_IWGRP | S_IROTH);
	index = 0;
	real_size=0;
	requested_size=0;
	update_stats(0,0);
}

//retourne l'adresse de la zone libre à droite la plus proche, retourne NULL si il n'y en a pas.
struct fb* trouver_voisin_droite(void * zone){
	struct fb * parcours = *((struct fb**) get_memory_adr());
	while(parcours != NULL && (void *) parcours  < zone){
		parcours = parcours->next;
	}
	return parcours;
}

//Renvoi l'adresse de la zone libre à gauche la plus proche de la zone "zone", NULL si il n'y en a pas.
struct fb* trouver_voisin_gauche(void * zone){
	struct fb * parcours = *((struct fb**) get_memory_adr());
	if(zone <= (void*)parcours)
		return NULL;
	struct fb * prec = parcours;
	while(parcours != NULL && (void *) parcours < zone){
		prec = parcours;
		parcours = parcours->next;
	}
	return prec;
}

void* mem_alloc(size_t size){
        void* mem = get_memory_adr();
        //On aligne size sur un multiple de 4
        size_t alignement = (size+sizeof(size_t))%4;
        size += alignement;

        //Size doit avoir une taille minimum, sinon probleme a la liberation
        if(size < sizeof(struct fb)){
                size = sizeof(struct fb);
        }
	real_size+=size+sizeof(size_t);
	requested_size+=size;
	update_stats(requested_size,real_size);

        //recuperation du bloc ou l'on pourra allouer notre zone
        struct fb* fb_current = (void*) fonction_recherche(*(struct fb**) mem, size);
        if (fb_current != NULL){
                struct fb * voisin_gauche = trouver_voisin_gauche((void*)fb_current);

                //Modification de la taille de la zone si necessaire : si on ne peut pas allouer de fb dans l'espace restant
                if(fb_current->size - size - sizeof(size_t) <= sizeof(struct fb)){
                    size = fb_current->size - sizeof(size_t);
                }
                //Si le bloc contigu a  droite est occupé : on ne peut pas créer de fb
                if(size == fb_current->size-sizeof(size_t)){
                    //Si on a  un voisin gauche
                    if(voisin_gauche != NULL){
                        voisin_gauche->next = (void *)fb_current->next;
                    }
                    else{
                        *(struct fb**)mem = (void *)fb_current->next;
                    }
                }
                //Si on peut allouer un fb apres la nouvelle zone allouee
                else{
                    struct fb* suivant = (void*)fb_current->next;
                    struct fb* fb_new = (void *)fb_current + sizeof(size_t) +size;
                    //Si on a un voisin gauche
                    if(voisin_gauche != NULL){
                        voisin_gauche->next = (void *)fb_new;
                    }
                    else{
                        *(struct fb**)mem = (void *)fb_new;
                    }
                    fb_new->size = fb_current->size - (size + sizeof(size_t));
                    fb_new->next = (void*)suivant;
                }
                *(size_t*)((void*)fb_current) = size;
                return (void *) fb_current + sizeof(size_t);
        }
        else{
                return NULL;
        }
}
//Affiche la chaine des blocs libres
void affiche_chainage(){
    struct fb* tete = *(struct fb**)get_memory_adr();
    while(tete != NULL){
        tete = tete->next;
    }
}

//Renvoi true si l'adresse zone est une adresse de debut de zone allouee
int est_liberable(void * zone){
        void* mem = get_memory_adr();
        struct fb* fb = *(struct fb**) mem;
        void* addr = mem + sizeof(struct fb*);
        size_t count = sizeof(struct fb**);
        int retour = 0;
        while(count < get_memory_size() && retour == 0){
                if(addr == (void *)fb){
                        addr = addr + fb->size;
                        count += fb->size;
                        fb = fb->next;
                } else {
                    if(addr+sizeof(size_t) == zone)
                        retour =1;
                    count += *(size_t*)addr + sizeof(size_t);
                    addr += *(size_t*)addr + sizeof(size_t);
                }
        }
        return retour;
}

void mem_free(void* zone){
    void* mem = get_memory_adr();
    //Adresse ou est stockee la valeur de la zone a liberer
    void * ptr_taille_zone = zone - sizeof(size_t);
    size_t taille_zone = *(size_t*) ptr_taille_zone;

    real_size-=(taille_zone+sizeof(size_t));
    requested_size-=taille_zone;
    update_stats(requested_size,real_size);

    size_t nouvelle_taille = taille_zone + sizeof(size_t);

    struct fb * voisin_gauche = trouver_voisin_gauche(zone);
    struct fb * voisin_droit = trouver_voisin_droite(zone);
    //Si il y a un voisin a gauche qui est colle a la zone qu'on veut liberer : fusionner avec lui
    if(voisin_gauche != NULL && (void*)voisin_gauche+voisin_gauche->size == ptr_taille_zone){
        nouvelle_taille += voisin_gauche->size;
        //Si la zone droite adjacente est libre : fusionner
        if((void *) voisin_droit == zone + taille_zone){
            nouvelle_taille+= voisin_droit->size;
            voisin_gauche->next = (void*)voisin_droit->next;
        }
        else{
                voisin_gauche->next = (void*)voisin_droit;
        }
        voisin_gauche->size = nouvelle_taille;
    }
    //Si on doit liberer le premier bloc disponible : il n'y a pas de voisin a gauche
    else if(voisin_gauche == NULL){
        struct fb* fb_new = (void*)ptr_taille_zone;
        //Si la zone droite adjacente est libre : fusionner
        if((void *) voisin_droit == zone + taille_zone){
                nouvelle_taille += voisin_droit->size;
                fb_new->next = (void*)voisin_droit->next;
        }
        else{
            fb_new->next = (void*)voisin_droit;
        }
        fb_new->size = nouvelle_taille;
        *(struct fb**) mem = (void *) fb_new;
    }
    //Si il y a un voisin a gauche mais pas colle a la zone a liberer
    else{
        struct fb* fb_new = ptr_taille_zone;
        //Si la zone droite adjacente est libre : fusionner
        if((void *) voisin_droit == zone + taille_zone){
            nouvelle_taille+= voisin_droit->size;
            fb_new->next = (void*)voisin_droit->next;
        }
        else{
            fb_new->next = (void*)voisin_droit;
        }
        fb_new->size = nouvelle_taille;
        voisin_gauche->next = (void*)fb_new;
    }
}

//Non implementee
size_t mem_get_size(void * zone){
        if(zone == NULL)
		return 0;
	else return *(size_t*)zone-1;
}

/* Iterateur sur le contenu de l'allocateur */
void mem_show(void (*print)(void * zone, size_t size, int free)){
        //affiche_chainage();
        void* mem = get_memory_adr();
        struct fb* fb = *(struct fb**) mem;
        void* addr = mem + sizeof(struct fb*);
        size_t count = sizeof(struct fb**);
        while(count < get_memory_size()){
                if(addr == (void *)fb){
                        print(addr, fb->size-sizeof(size_t), 1);
                        addr = addr + fb->size;
                        count += fb->size;
                        fb = fb->next;
                } else {
                        print(addr, *(size_t*)addr, 0);
                        count += *(size_t*)addr + sizeof(size_t);
                        addr += *(size_t*)addr + sizeof(size_t);
                }
        }
}



void mem_fit(mem_fit_function_t* function){
    fonction_recherche = function;
}

void switch_strategy(){
	fflush(stdout);
	char commande;
	fprintf(stderr,"Donnez la stratégie que vous souhaitez utiliser :\n\t- f pour first fit\n\t- b pour best fit\n\t- w pour worst fit\n");	
	printf("? ");
	fflush(stdout);
	commande = getchar(); //MDR C BO LA VIE
	commande = getchar();
	printf("My char is %c\n",commande);
	switch (commande) {
		case 'f':
			fprintf(stderr,"Vous avez choisi d'utiliser la stratégie first fit !\n");
			mem_fit(&mem_fit_first);
		break;
		case 'b':
			fprintf(stderr,"Vous avez choisi d'utiliser la stratégie best fit !\n");
			mem_fit(&mem_fit_best);
		break;
		case 'w':
			fprintf(stderr,"Vous avez choisi d'utiliser la stratégie worst fit !\n");
			mem_fit(&mem_fit_worst);
		break;
		default:
			fprintf(stderr,"Vous avez choisi d'utiliser la stratégie first fit !\n");
			mem_fit(&mem_fit_first);
		break;
	}

}

//Renvoi l'adresse de la premiere zone ou l'on peut allouer
struct fb* mem_fit_first(struct fb* list, size_t size){
        while(list != NULL && list->size < size+sizeof(size_t)){
                list = list->next;
        }
        return list;
}

/* Si vous avez le temps */
struct fb* mem_fit_best(struct fb* list, size_t size){
        size_t best;
        struct fb * retour;
        if(list != NULL){
                best = get_memory_size();
                retour = NULL;
        }
        else{
                return NULL;
        }
        while(list != NULL){
                if(list->size >= size+sizeof(size_t) && best > list->size){
                        best = list->size;
                        retour = list;
                }
                list = list->next;
        }
        return retour;
}


struct fb* mem_fit_worst(struct fb* list, size_t size){
        size_t worst;
        struct fb * retour;
        if(list != NULL){
                worst = 0;
                retour = NULL;
        }
        else{
                return NULL;
        }
        while(list != NULL){
                if(list->size >= size+sizeof(size_t) && worst < list->size){
                        worst = list->size;
                        retour = list;
                }
                list = list->next;
        }
	//printf("retour : %p\n",retour);
        return retour;
}
