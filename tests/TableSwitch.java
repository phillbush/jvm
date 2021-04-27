public class TableSwitch {
	public TableSwitch() {
		System.out.println("nop");
	}

	public static int choose(int n) {
		switch (n) {
		case 0:
			return 0;
		case 1:
			return 1;
		case 2:
			return 2;
		}
		return -1;
	}

	public static void main(String[] args) {
		System.out.println(choose(-1));
		System.out.println(choose(0));
		System.out.println(choose(1));
		System.out.println(choose(2));
		System.out.println(choose(3));
	}
}
