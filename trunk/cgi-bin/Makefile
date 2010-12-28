all: cover.o add_order add_shop add_web delete_order delete_shop \
     delete_web edit_order edit_shop edit_web error \
     handle_order make_add_order make_add_shop \
     make_edit_order make_edit_shop make_edit_web \
     search state_year top_five trade_balance \
     view_order view_shop view_web which_area \
     which_month which_order which_shop which_web which_year

CC = gcc

CFLAGS = -O3

cover.o : cover.c
	$(CC) cover.c -c $(CFLAGS)

add_order : add_order.c cover.o
	$(CC) add_order.c cover.o -o add_order $(CFLAGS)

add_shop : add_shop.c cover.o
	$(CC) add_shop.c cover.o -o add_shop $(CFLAGS)

add_web : add_web.c cover.o
	$(CC) add_web.c cover.o -o add_web $(CFLAGS)

delete_order : delete_order.c cover.o
	$(CC) delete_order.c cover.o -o delete_order $(CFLAGS)

delete_shop : delete_shop.c cover.o
	$(CC) delete_shop.c cover.o -o delete_shop $(CFLAGS)

delete_web : delete_web.c cover.o
	$(CC) delete_web.c cover.o -o delete_web $(CFLAGS)

edit_order : edit_order.c cover.o
	$(CC) edit_order.c cover.o -o edit_order $(CFLAGS)

edit_shop : edit_shop.c cover.o
	$(CC) edit_shop.c cover.o -o edit_shop $(CFLAGS)

edit_web : edit_web.c cover.o
	$(CC) edit_web.c cover.o -o edit_web $(CFLAGS)

error : error.c
	$(CC) error.c -o error $(CFLAGS)

handle_order : handle_order.c cover.o
	$(CC) handle_order.c cover.o -o handle_order $(CFLAGS)

make_add_order : make_add_order.c cover.o
	$(CC) make_add_order.c cover.o -o make_add_order $(CFLAGS)

make_add_shop : make_add_shop.c cover.o
	$(CC) make_add_shop.c cover.o -o make_add_shop $(CFLAGS)

make_edit_order : make_edit_order.c cover.o
	$(CC) make_edit_order.c cover.o -o make_edit_order $(CFLAGS)

make_edit_shop : make_edit_shop.c cover.o
	$(CC) make_edit_shop.c cover.o -o make_edit_shop $(CFLAGS)

make_edit_web : make_edit_web.c cover.o
	$(CC) make_edit_web.c cover.o -o make_edit_web $(CFLAGS)

search : search.c cover.o
	$(CC) search.c cover.o -o search $(CFLAGS)

state_year : state_year.c cover.o
	$(CC) state_year.c cover.o -o state_year $(CFLAGS)

top_five : top_five.c cover.o
	$(CC) top_five.c cover.o -o top_five $(CFLAGS)

trade_balance : trade_balance.c cover.o
	$(CC) trade_balance.c cover.o -o trade_balance $(CFLAGS)

view_order : view_order.c cover.o
	$(CC) view_order.c cover.o -o view_order $(CFLAGS)

view_shop : view_shop.c cover.o
	$(CC) view_shop.c cover.o -o view_shop $(CFLAGS)

view_web : view_web.c cover.o
	$(CC) view_web.c cover.o -o view_web $(CFLAGS)

which_area : which_area.c cover.o
	$(CC) which_area.c cover.o -o which_area $(CFLAGS)

which_month : which_month.c cover.o
	$(CC) which_month.c cover.o -o which_month $(CFLAGS)

which_order : which_order.c cover.o
	$(CC) which_order.c cover.o -o which_order $(CFLAGS)

which_shop : which_shop.c cover.o
	$(CC) which_shop.c cover.o -o which_shop $(CFLAGS)

which_web : which_web.c cover.o
	$(CC) which_web.c cover.o -o which_web $(CFLAGS)

which_year : which_year.c cover.o
	$(CC) which_year.c cover.o -o which_year $(CFLAGS)

clean :
	-rm *.o

